# looseleaf
A simple, single-header drawing library, rooted in binary trees

> 🚧 This project is under construction!

## What will this project entail?
Once complete, looseleaf will be a simple and flexible tool for designing immediate-mode user interfaces. It is _not_ aimed at flexbox-type layouts: instead, text boxes, images, and other UI elements are laid out in a simple binary tree structure using node combinators. This allows interfaces to be constructed in a functional style: rather than committing elements to the screen with loops, nodes are constructed and combined with list operations. 

For the functional nerds out there, looseleaf's coolness comes from the fact that nodes are monoidal: `LL_EMPTY_LEAF` is the unit, and the binary node combinators are monoidal products! Basically, what this means is that there awesome ways to fold up lists of UI elements. 

## Node combinators
Some of the node combinators looseleaf offers include:

- `ll_overlay`, which overlays its first child on top of its second
- `ll_beside`, which aligns the right edge of its first child with the left edge of its second
- `ll_above`, which aligns the bottom edge of its first child with the top edge of its second

Each combinator also accepts a configuration, allowing for the tuning of node alignment, offset, and other parameters. 

## Example
Although the design of looseleaf's API is subject to change, below is an example program to demonstrate how it might work.

```c
#define SIZE 4096

int main() {
  char arena[SIZE];
  ll_Context* ctx = ll_init(arena, SIZE);
  ll_begin(ctx);

  ll_NodeHandle im = ll_image(NULL, {.width = 1, .height = 1});

  // nesting multiple node combinators
  ll_NodeHandle overlay_demo = ll_overlay(
    {.align_h = LL_HORIZ_ALIGN_LEFT},
    ll_text({.letter_spacing = 3}, "hello world"),
    ll_beside({.align_v = LL_VERT_ALIGN_CENTER}, im, im)
  );

  // will draw something like this...

  // +----------+----------+
  // |          |          |
  // hello, world!         |
  // |          |          |
  // +----------+----------+

  // folding over an array
  const ll_NodeHandle messages[3];
  messages[0] = ll_text({}, "zero");
  messages[1] = ll_text({}, "one");
  messages[2] = ll_text({}, "two");
  ll_NodeHandle align_demo = LL_FOLDL1(ll_above, {.align_h = LL_HORIZ_ALIGN_RIGHT}, messages);

  // will draw something like this...

  //  zero
  //   one
  //   two

  ll_NodeHandle complete_demo = ll_overlay(
    {.align_h = LL_HORIZ_ALIGN_RIGHT, .align_v = LL_VERT_ALIGN_BOTTOM},
    align_demo,
    overlay_demo
  );

  // will draw something like this...

  // +----------+----------+
  // |          |          |
  // hello, world!      zero
  // |          |        one
  // +----------+--------two

  ll_RenderCommandArray cmds = ll_gen_commands(complete_demo);
}
```

## Theory of operation
The core looseleaf header is dependency-free, and therefore will not contain any platform-specific rendering code. Similar to other immediate-mode UI libraries such as Clay, it outputs an array of render commands, which the backend can iterate to render the UI. looseleaf will provide extensions for various backends (SDL, LovyanGFX, etc.), not only for rendering, but also for access to implementation-specific information such as text and image sizing. 

Each time a new node is created, whether it is a combinator or a leaf, looseleaf allocates the node in its internal memory arena and returns an opaque handle (`ll_NodeHandle`) that can be supplied in future allocations. The arena is wiped clean every time the user calls `ll_begin(ctx)`. To ensure that "dirty" node handles are never used, the looseleaf context keeps track of its generation, and each handle tracks the generation it was created in. If there is a mismatch, looseleaf will politely refuse to render. 
