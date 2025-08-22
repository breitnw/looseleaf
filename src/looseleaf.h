// looseleaf.h: a simple drawing library, rooted in binary trees

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// HEADER ======================================================================

// initialization stage ========================================================
// --> create the memory arena and context

typedef struct ll_Context ll_Context;

ll_Context ll_init(char* arena_mem, size_t arena_capacity);

// setup stage =================================================================
// --> initialize ephemeral memory and prepare the context

void ll_begin_record(ll_Context* ctx);

// recording stage =============================================================
// --> instantiate nodes and add them to the context

// helper data -----------------------------------------------------------------

typedef enum {
  LL_HORIZ_ALIGN_LEFT,
  LL_HORIZ_ALIGN_CENTER,
  LL_HORIZ_ALIGN_RIGHT,
} ll_HorizAlign;

typedef enum {
  LL_VERT_ALIGN_TOP,
  LL_VERT_ALIGN_CENTER,
  LL_VERT_ALIGN_BOTTOM,
} ll_VertAlign;

typedef struct {
  uint32_t width;
  uint32_t height;
} ll_Size;

typedef struct {
  int32_t x, y;
} ll_Vec2;

// types of nodes --------------------------------------------------------------

// configuration for each node type --------------------------------------------

typedef uint32_t ll_NodeHandle;

typedef struct {
  const char* text;
  // Additional spacing between letters (can be negative)
  int16_t letter_spacing; // don't really need support for newlines
} ll_TextConfig;

// TODO move this
// Provided a single line of text and a pixel spacing between letters, return
// the dimensions of that line in pixels.
ll_Size ll_measure_text(const char* text, uint16_t letter_spacing);

typedef struct {
  ll_HorizAlign align_h;
  ll_Vec2 offset;
} ll_AboveConfig;

typedef struct {
  ll_VertAlign align_v;
  ll_Vec2 offset;
} ll_BesideConfig;

typedef struct {
  ll_HorizAlign align_h;
  ll_VertAlign align_v;
  ll_Vec2 offset;
} ll_OverlayConfig;

typedef struct {
  ll_Vec2 offset;
} ll_MovePinholeConfig;

typedef struct ll__Node {
  // A union describing the required data of a node.
  // A node's data is the required information about its children, or the
  // specific image or text that will be rendered.
  union Data {
    // The data of this node, if it is an image node
    struct ImageData {
      void *image_data;
      ll_Size image_size;
    };
    // The text contents of this node, if it is a text node
    const char* text_data;
    // the single child of this node, if it is a transformation
    ll_NodeHandle child;
    // the two children of this node, if it is a combinator
    struct Children {
      // The first child of this node, if one exists
      ll_NodeHandle first_child;
      // The second child of this node, if one exists
      ll_NodeHandle second_child;
    } children;
  } data;

  // A union describing the configuration of a node. A node's configuration
  // is the optional styling information specific to its type.
  union Config {
    ll_TextConfig text_config;
    ll_AboveConfig above_config;
    ll_BesideConfig beside_config;
    ll_OverlayConfig overlay_config;
    ll_MovePinholeConfig move_pinhole_config;
  } config;

  // TODO consolidate the two unions above into one

  // An enum describing a node's type.
  enum Tag {
    LL__NODE_TYPE_IMAGE,
    LL__NODE_TYPE_TEXT,
    LL__NODE_TYPE_ABOVE,
    LL__NODE_TYPE_BESIDE,
    LL__NODE_TYPE_OVERLAY,
    LL__NODE_TYPE_MOVE_PINHOLE,
    LL__NODE_TYPE_RESET_PINHOLE,
  } tag;
} ll__Node;

typedef struct ll__NodeArray {
  // the total underlying capacity of the array
  uint32_t capacity;
  // the number of initialized render commands
  uint32_t length;
  // a pointer to the first element of the array in memory
  ll__Node* internalArray;
} ll__NodeArray;

// child nodes always come last as parameters, in case function calls are nested
ll_NodeHandle ll_image(void* image_data);
ll_NodeHandle ll_text(const char* text, ll_TextConfig conf);
ll_NodeHandle ll_above(ll_AboveConfig conf, ll_NodeHandle above, ll_NodeHandle below);
ll_NodeHandle ll_beside(ll_BesideConfig conf, ll_NodeHandle left, ll_NodeHandle right);
ll_NodeHandle ll_overlay(ll_OverlayConfig conf, ll_NodeHandle over, ll_NodeHandle under);
ll_NodeHandle ll_move_pinhole(ll_MovePinholeConfig conf);
ll_NodeHandle ll_reset_pinhole();

// draw finalization stage: convert node tree to iterable array ================

// Tag representing a type of render command
typedef enum {
  LL_RENDER_DATA_TAG_IMAGE,
  LL_RENDER_DATA_TAG_TEXT,
} ll_RenderDataTag;

// Data for rendering an
typedef struct {
  void* imageData;
} ll_ImageRenderData;

typedef struct {
  const char* text;
} ll_TextRenderData;

typedef union {
  ll_ImageRenderData image_render_data;
  ll_TextRenderData text_render_data;
} ll_RenderDataUnion;

typedef struct {
  int32_t x, y;
  uint32_t w, h;
} ll_Bounds;

typedef struct {
  ll_RenderDataTag tag;
  ll_Bounds bounds;
  ll_RenderDataUnion render_data;
} ll_RenderCommand;

// An array of render commands
typedef struct {
  // the total underlying capacity of the array
  uint32_t capacity;
  // the number of initialized render commands
  uint32_t length;
  // a pointer to the first element of the array in memory
  ll_RenderCommand* internalArray;
} ll_RenderCommandArray;

ll_RenderCommandArray ll_end_record();

// data ------------------------------------------------------------------------

typedef struct ll__Arena {
  uintptr_t next_alloc;
  size_t capacity;
  char* mem;
} ll__Arena;

struct ll_Context {
  uint32_t max_nodes;
  ll__Arena arena;
  ll__NodeArray nodes;
};

// IMPLEMENTATION ==============================================================

ll_Context ll_init(char* arena_mem, size_t arena_capacity) {
  ll__Arena arena = (ll__Arena){
      .capacity = arena_capacity,
      .mem = arena_mem,
  };

  return (ll_Context) { .arena = arena, };
}

int main() {
  #define SIZE 10000
  char arena[SIZE];
  ll_Context ctx = ll_init(arena, SIZE);
  ll_begin_record(&ctx); // TODO rename to "free"?

  // TODO it seems that some of these parameters aren't required but should be...
  // may need to split up "config" and "data" for node types
  ll_NodeHandle im = ll_image({.image_data = NULL, .width = 1, .height = 1});
  ll_NodeHandle over = ll_overlay(
      {.align_h = LL_HORIZ_ALIGN_LEFT},
      ll_text({.text = "hello world!"}),
      ll_beside({.align_h = LL_HORIZ_ALIGN_CENTER}, im, im)
  );

  ll_end_record(); // TODO rename to "to_commands"?
}
