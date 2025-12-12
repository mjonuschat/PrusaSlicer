#include "Slic3r/Biz/Expr/Simplify.hpp"

#include <vector>
#include <algorithm>
#include <boost/variant/get.hpp>

namespace Slic3r::Biz::Expr {

namespace {

using namespace Slic3r::Domain::Expr;

class Simplifier : public boost::static_visitor<ExprAst>
{
public:
    static ExprAst simplify(const ExprAst& ast)
    {
        Simplifier visitor;
        return boost::apply_visitor(visitor, ast);
    }

    // --- Base Cases ---
    template <typename T>
    ExprAst operator()(const T& val) const
    {
        return val;
    }

    ExprAst operator()(const Unary& u) const
    {
        ExprAst simple_expr = boost::apply_visitor(*this, u.expr);

        // Double negation: !!A -> A
        if (u.op == UnaryOp::Not) {
            if (auto* child = boost::get<Unary>(&simple_expr)) {
                if (child->op == UnaryOp::Not)
                    return child->expr;
            }
        }
        return Unary(u.op, std::move(simple_expr));
    }

    ExprAst operator()(const FuncCall& f) const
    {
        FuncCall new_f = f;
        for (auto& arg : new_f.args)
            arg = boost::apply_visitor(*this, arg);
        return new_f;
    }

    ExprAst operator()(const Binary& b) const
    {
        // 1. First, simplify children recursively
        ExprAst left  = boost::apply_visitor(*this, b.left);
        ExprAst right = boost::apply_visitor(*this, b.right);

        // 2. Logic Chain Optimization (AND/OR)
        if (b.op == BinaryOp::And || b.op == BinaryOp::Or) {
            return simplify_logic_chain(b.op, std::move(left), std::move(right));
        }

        return Binary(b.op, std::move(left), std::move(right));
    }

private:
    // Flattens a tree of (A && (B && C)) into a vector {A, B, C}
    void collect_operands(const ExprAst& node, BinaryOp op, std::vector<ExprAst>& list) const
    {
        if (const Binary* b = boost::get<Binary>(&node)) {
            if (b->op == op) {
                collect_operands(b->left, op, list);
                collect_operands(b->right, op, list);
                return;
            }
        }
        list.push_back(node);
    }

    // The heavy lifting: Idempotency and Absorption
    ExprAst simplify_logic_chain(BinaryOp op, ExprAst left, ExprAst right) const
    {
        std::vector<ExprAst> terms;

        // 1. Flatten
        // (We construct a temp binary just to use collect_operands helper,
        // avoiding code duplication)
        Binary temp_node(op, std::move(left), std::move(right));
        collect_operands(temp_node, op, terms);

        // 2. Remove Duplicates (Idempotency: A && A -> A)
        // Since we don't have a strict ordering < operator, we do O(N^2) scan.
        // N is usually very small in logic expressions (< 10).
        auto new_end = std::unique(
            terms.begin(),
            terms.end(),
            [](const ExprAst& a, const ExprAst& b) { return equals_to(a, b); }
        );
        terms.erase(new_end, terms.end());

        // 3. Remove "Absorbed" terms
        // Pattern: A && (A || B) -> A
        // We look for terms that "eat" other terms.
        BinaryOp anti_op = (op == BinaryOp::And) ? BinaryOp::Or : BinaryOp::And;

        std::vector<ExprAst> final_terms;
        std::vector<bool> absorbed(terms.size(), false);

        for (size_t i = 0; i < terms.size(); ++i) {
            if (absorbed[i])
                continue;

            for (size_t j = 0; j < terms.size(); ++j) {
                if (i == j || absorbed[j])
                    continue;

                // Check if terms[j] is a complex expression containing terms[i]
                // Example: terms[i] = A
                // terms[j] = (A || B) (Binary node with op == anti_op)
                if (const Binary* bin_j = boost::get<Binary>(&terms[j])) {
                    if (bin_j->op == anti_op) {
                        // Check strict absorption: Does bin_j contain terms[i] directly?
                        // Note: This checks one level deep. For deep absorption, collect_operands on bin_j too.
                        if (equals_to(terms[i], bin_j->left) || equals_to(terms[i], bin_j->right)) {
                            // terms[j] is redundant.
                            // In AND chain: A && (A || B) -> Keep A, drop (A||B)
                            // In OR chain:  A || (A && B) -> Keep A, drop (A&&B)
                            absorbed[j] = true;
                        }
                        // Handle (B || A) case (commutative check)
                        // Handled above by checking both left and right.
                    }
                }
            }
        }

        for (size_t i = 0; i < terms.size(); ++i) {
            if (!absorbed[i])
                final_terms.push_back(std::move(terms[i]));
        }

        // 4. Rebuild the tree
        if (final_terms.empty()) {
            // Should not happen unless input was empty, logical fallback
            return (op == BinaryOp::And) ? true : false;
        }

        ExprAst result = std::move(final_terms[0]);
        for (size_t i = 1; i < final_terms.size(); ++i) {
            result = Binary(op, std::move(result), std::move(final_terms[i]));
        }

        return result;
    }
};

} // namespace

ExprAst simplify(const ExprAst& expr)
{
    return Simplifier::simplify(expr);
}

} // namespace Slic3r::Biz::Expr
