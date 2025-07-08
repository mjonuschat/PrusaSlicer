#include "Slic3r/Biz/Arrange/Arrange.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp"
#include "Slic3r/Biz/Arrange/Kernels/GravityKernel.hpp"
#include "Slic3r/Biz/Arrange/Kernels/IKernel.hpp"
#include "Slic3r/Biz/Arrange/Kernels/RectangleOverfitKernelWrapper.hpp"
#include "Slic3r/Biz/Arrange/Kernels/SVGDebugOutputKernelWrapper.hpp"
#include "Slic3r/Biz/Arrange/Kernels/TMArrangeKernel.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Arrange/Packer.hpp"
#include "libslic3r/Optimize/NLoptOptimizer.hpp"
#include "libslic3r/Optimize/Optimizer.hpp"

namespace Slic3r::Biz::Arrange {

#ifndef NDEBUG
constexpr bool use_debug_kernel{true};
#else
constexpr bool use_debug_kernel{false};
#endif

using Algorithms::BoundingBox::center;
using Algorithms::BoundingBox::merge;
using Algorithms::BoundingBox::sizes;
using Algorithms::BoundingBox::unscaled;
using Algorithms::Scaling::scaled;
using Domain::BoundingBox2crd;
using Domain::BoundingBox2d;
using Domain::Index2;
using Domain::Vec2crd;
using Domain::Vec2d;

namespace {
Domain::BoundingBox2crd get_extents(const std::vector<ArrangeItem>& items)
{
    ASSERT(!items.empty());

    BoundingBox2crd result{items.front().fixed_shape().bounding_box()};
    for (const ArrangeItem& item : std::span{items}.subspan(1)) {
        result = merge(result, item.fixed_shape().bounding_box());
    }
    return result;
}

BoundingBox2d get_occupied_segments_bb(
    const BoundingBox2d& bed_bounding_box,
    const Vec2d& occupied_segments_bb_sizes,
    const PivotPoint pivot_point
)
{
    switch (pivot_point) {
    case PivotPoint::Center: {
        return {
            Vec2d{center(bed_bounding_box) - occupied_segments_bb_sizes / 2.0},
            Vec2d{center(bed_bounding_box) + occupied_segments_bb_sizes / 2.0}
        };
    }
    case PivotPoint::BottomLeft: {
        return {bed_bounding_box.min, bed_bounding_box.min + occupied_segments_bb_sizes};
    }
    case PivotPoint::BottomRight: {
        const Vec2d right_bottom_point{bed_bounding_box.min + Vec2d{sizes(bed_bounding_box).x(), 0.0}};
        return {
            right_bottom_point - Vec2d{occupied_segments_bb_sizes.x(), 0.0},
            right_bottom_point + Vec2d{0.0, occupied_segments_bb_sizes.y()}
        };
    }
    case PivotPoint::TopLeft: {
        const Vec2d top_left_point{bed_bounding_box.min + Vec2d{0.0, sizes(bed_bounding_box).y()}};
        return {
            top_left_point - Vec2d{0.0, occupied_segments_bb_sizes.y()},
            top_left_point + Vec2d{occupied_segments_bb_sizes.x(), 0.0}
        };
    }
    case PivotPoint::TopRight: {
        return {bed_bounding_box.max - occupied_segments_bb_sizes, bed_bounding_box.max};
    }
    }
    PANIC("Unknown pivot point");
}

void align_pile(std::vector<ArrangeItem>& items, const RectangleBed& bed)
{
    if (items.empty()) {
        return;
    }

    const BoundingBox2d bed_bounding_box{unscaled<double>(bed.bounding_box())};
    const BoundingBox2d pile_bounding_box{unscaled<double>(get_extents(items))};
    const Vec2d segment_size{
        sizes(bed_bounding_box).array() / Vec2d{bed.segments().x_count, bed.segments().y_count}.array()
    };

    ASSERT((sizes(pile_bounding_box).array() < sizes(bed_bounding_box).array()).all());

    const Domain::Index2 occupied_segments_count{
        static_cast<int>(std::ceil(sizes(pile_bounding_box).x() / segment_size.x())),
        static_cast<int>(std::ceil(sizes(pile_bounding_box).y() / segment_size.y()))
    };

    const Vec2d occupied_segments_bb_sizes{
        occupied_segments_count[0] * segment_size.x(),
        occupied_segments_count[1] * segment_size.y(),
    };

    const BoundingBox2d occupied_segments_bb{
        get_occupied_segments_bb(bed_bounding_box, occupied_segments_bb_sizes, bed.pivot_point())
    };

    const Vec2d translation{center(occupied_segments_bb) - center(pile_bounding_box)};

    for (ArrangeItem& item : items) {
        item.set_translation(item.get_translation() + scaled(translation));
    }
}

std::vector<ArrangeItem> to_arrange_items(const std::vector<ArbitraryShape>& items, const Settings& settings)
{
    std::vector<ArrangeItem> result;
    for (const ArbitraryShape& shape : items) {
        result.push_back(ArrangeItem{shape, settings});
    }
    return result;
}
} // namespace

ArrangeResult arrange(
    const IBed& bed,
    const std::vector<ArbitraryShape>& items,
    const std::vector<ArbitraryShape>& fixed_items,
    const Settings& settings
)
{
    std::vector<ArrangeItem> arrange_items{to_arrange_items(items, settings)};
    std::unique_ptr<Kernels::IKernel> base_kernel;
    if (dynamic_cast<const CircleBed*>(&bed) != nullptr || settings.strategy == Strategy::PullToCenter)
    {
        base_kernel = std::make_unique<Kernels::GravityKernel>();
    } else {
        base_kernel = std::make_unique<Kernels::TMArrangeKernel>(arrange_items.size(), bed.area());
    }

    const bool with_wipe_tower{std::ranges::any_of(arrange_items, [](const ArrangeItem& item) {
        return item.is_wipe_tower;
    })};

    std::unique_ptr<Kernels::IKernel> final_kernel;
    const InfiniteBed infinite_bed{center(bed.bounding_box())};
    const IBed* final_bed{nullptr};

    const bool use_overfit{
        !with_wipe_tower
        && settings.strategy == Strategy::Auto
        && dynamic_cast<const RectangleBed*>(&bed) != nullptr
        && fixed_items.empty()
    };

    if (use_overfit) {
        final_kernel = std::make_unique<Kernels::RectangleOverfitKernelWrapper>(
            std::move(base_kernel),
            bed.bounding_box()
        );
        final_bed = &infinite_bed;
    } else {
        final_kernel = std::move(base_kernel);
        final_bed    = &bed;
    }

    if constexpr (use_debug_kernel) {
        final_kernel = std::make_unique<Kernels::SVGDebugOutputKernelWrapper>(
            bed.bounding_box(),
            std::move(final_kernel)
        );
    }

    constexpr double accuracy{1.0};
    Packer packer{std::move(final_kernel), accuracy, opt::Optimizer<opt::AlgNLoptSubplex>{}, []() {
        return false;
    }};

    ArrangeResult result;
    PackingContext context;
    context.add_fixed_items(to_arrange_items(fixed_items, settings));
    for (std::size_t i{}; i < arrange_items.size(); ++i) {
        ArrangeItem& item{arrange_items[i]};
        const bool packed{
            packer.pack(*final_bed, item, context, std::span{arrange_items}.subspan(i + 1))
        };
        if (packed) {
            context.add_packed_item(item);
            result.packed.push_back(item);
        } else {
            result.not_packed.push_back(item);
        }
    }

    if (use_overfit) {
        align_pile(result.packed, dynamic_cast<const RectangleBed&>(bed));
    }
    return result;
}
} // namespace Slic3r::Biz::Arrange
