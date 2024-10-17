# Doxygen guide

## Creating relevant targets

To add doxygen generation related targets for your cmake target, just in your `CMakeLists.txt` call like:
```
add_doxygen_target(slic3r-shared)
```
(note: replace the `slic3r-shared` with your target).

The `add_doxygen_target` is defined in `cmake/modules/DoxygenTarget.cmake` which is included in top-level CMakeLists.txt
file and hence it is available to all CMakeLists.txt files in subdirectories.

This function will for given target create:
- `${target_name}-doxygen-tag` target to create index of documented elements (to be used by other libs docs to cross-link docs)
- `${target_name}-doxygen` to generate the doxygen docs

It also registers all created `${target_name}-doxygen` targets as dependency for `slic3r-doxygen` target to be used to 
generate documentation for all libs.

## Doxygen comment styles

This document outline style of doxygen comments.

In points:
1. Use doxygen commands prefixed with `@` (i.e. use `@brief` instead of `\brief`),
2. Place doxygen comments only into header files
3. Each documented element (class, function, file, global variable etc.) should have at least `@brief` description
4. Use `@param` to document function parameters (if you want to write multiple paragraphs related to specified parameter, 
   enclose the paragraphs within `@parblock` and `@endparblock` commands).
5. Use `@tparam` to doucment template parameters
6. Use `@return` to document
7. If set of free functions is placed in a file, consider using `@file <filename>` section to describe the module, 
   and allow cross-linking by `@ref <filename> "link text"` or `@see <filename>`.
8. For formatting [use makrdown](https://www.doxygen.nl/manual/markdown.html) (instead of doxygen commands)
9. Note that [lists](https://www.doxygen.nl/manual/lists.html) has to be ended with dot on separate line like this:
   ```c++
   /**
    * This is sort of low-level object, for more comfort use
    * - Scene as entry point to scenegraph,
    * - @ref NodeVisitor.hpp "node visitors" to visit and transform (sub-)graph
    * - NodeBuilder to create sub-scenegraph
    * .
    *
    * In terms of tree hierarchy node contains:
    * - parent link: see @ref Node::parent() const
    * - list of children (see Node::children() const
    * .
    */     
   ```
10. If you want to group elements in a structure  (class, struct, enum), you can use `@name`, `@{`, `@}` like this:
    ```c++
    class Node 
    {
        //...
   
        /**
         * @name TransformModifier
         * World Transformation modifier
         * @{
          */
        const INodeTransformModifier* transform_modifier() const { return m_transform_modifier.get(); }
       INodeTransformModifier* transform_modifier() { return m_transform_modifier.get(); }
        void set_transform_modifier(std::unique_ptr<INodeTransformModifier>&& modifier)
        { m_transform_modifier = std::move(modifier); }
        /**@}*/
       
        //...
    }
   
   ```

Example:

```c++

```