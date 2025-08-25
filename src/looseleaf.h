// looseleaf.h: a simple drawing library, rooted in binary trees

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//    +----------+
//   /  HEADER  /
// -+----------+----------------------------------------------------------------

// TODO find a better place for this
#ifndef LL_IMAGE_TYPE
#define LL_IMAGE_TYPE void
#endif

// initialization stage ========================================================
// --> create the memory arena and context

typedef struct ll_Context ll_Context;

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

typedef uint32_t ll_NodeHandle;

// HACK no support for newlines as of now
typedef struct {
  // Additional spacing between letters (can be negative)
  int16_t letter_spacing;
} ll_TextConfig;

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

// the node struct -------------------------------------------------------------

typedef struct ll__Node {
  // A union describing the contents (image, text, or children) of a node.
  union Contents {
    // The data of this node, if it is an image node
    struct ImageData {
      LL_IMAGE_TYPE* image_data;
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

  // A union describing the optional configuration of a node.
  union Config {
    ll_TextConfig text_config;
    ll_AboveConfig above_config;
    ll_BesideConfig beside_config;
    ll_OverlayConfig overlay_config;
    ll_MovePinholeConfig move_pinhole_config;
  } config;

  // TODO consolidate the two unions above into one?

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


// draw finalization stage =====================================================
// --> convert node tree to iterable array

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
  ll_Vec2 posn;
  ll_Size size;
} ll_Bounds;

typedef struct {
  ll_Bounds bounds;
  ll_RenderDataTag tag;
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


// context data ================================================================

typedef struct {
  uintptr_t next_alloc;
  size_t capacity;
  char* mem;
} ll__Arena;

struct ll_Context {
  uint32_t max_nodes;
  ll__Arena arena;
  ll__NodeArray nodes;
};


// public function API =========================================================

// setup and initialization...

// Configure the function looseleaf uses to measure text.
// Required before creating a context.
void ll_set_text_measurement_fn(ll_Size (*text_measurement_fn)(const char* text, uint16_t letter_spacing));
// Configure the function looseleaf uses to measure images.
// Required before creating a context.
void ll_set_image_measurement_fn(ll_Size (*image_measurement_fn)(LL_IMAGE_TYPE* image));
// Configure the maximum number of nodes that can be "in flight" at a given time
void ll_configure_max_nodes(uint32_t max_nodes);
// Return the minimum size of an arena used to initialize the looseleaf context
uint64_t ll_min_arena_size(void);
// Initialize a looseleaf context from a memory arena
// TODO error if measurement functions aren't set up properly
ll_Context* ll_init(char* arena_mem, size_t arena_capacity);

// per-frame recording...

// Clear the looseleaf context and set it up for recording
void ll_begin(ll_Context* ctx);

// DESIGN: data comes after configuration, in case function calls are nested

// Allocate a leaf representing an image, with data defined by LL_IMAGE_TYPE
ll_NodeHandle ll_image(LL_IMAGE_TYPE* image_data, ll_Size image_size);
// Allocate a leaf represeting a string of text
ll_NodeHandle ll_text(ll_TextConfig conf, const char* text);
// Allocate a binary node that renders the first node above the second
ll_NodeHandle ll_above(ll_AboveConfig conf, ll_NodeHandle above, ll_NodeHandle below);
// Allocate a binary node that renders the first node to the left of the second
ll_NodeHandle ll_beside(ll_BesideConfig conf, ll_NodeHandle left, ll_NodeHandle right);
// Allocate a binary node that renders the first node on top of the second
ll_NodeHandle ll_overlay(ll_OverlayConfig conf, ll_NodeHandle over, ll_NodeHandle under);
// Allocate a unary node that moves a node's pinhole as defined by `conf`
ll_NodeHandle ll_move_pinhole(ll_MovePinholeConfig conf, ll_NodeHandle node);
// Allocate a unary node that resets a node's pinhole to its original position
ll_NodeHandle ll_reset_pinhole(ll_NodeHandle node);

// Generate an iterable array of render commands from an ll_NodeHandle
ll_RenderCommandArray ll_gen_commands(ll_NodeHandle root);


//    +------------------+
//   /  IMPLEMENTATION  /
// -+------------------+--------------------------------------------------------

// program state (ugly, gross, disgraceful) ====================================

ll_Context* ll__current_context;
uint32_t ll__max_nodes = 4096;

// private functions ===========================================================

// TODO find a home for these

// Provided a single line of text and a pixel spacing between letters, return
// the dimensions of that line in pixels.
ll_Size ll__measure_text(const char* text, uint16_t letter_spacing);

// Provided an instance of LL_IMAGE_TYPE, return the pixel size of that image
ll_Size ll__measure_image(LL_IMAGE_TYPE* image);

// public functions ============================================================

ll_Context* ll_init(char* arena_mem, size_t arena_capacity) {
  ll__Arena arena = (ll__Arena){
      .capacity = arena_capacity,
      .mem = arena_mem,
  };

  return (ll_Context) { .arena = arena, };
}


// EXAMPLE =====================================================================

#define SIZE 4096

int main() {
  char arena[SIZE];
  ll_Context ctx = ll_init(arena, SIZE);
  ll_begin(&ctx);

  ll_NodeHandle im = ll_image(NULL, {.width = 1, .height = 1});
  ll_NodeHandle over = ll_overlay(
      {.align_h = LL_HORIZ_ALIGN_LEFT},
      ll_text({.letter_spacing = 3}, "hello world"),
      ll_beside({.align_v = LL_VERT_ALIGN_CENTER}, im, im)
  );

  ll_RenderCommandArray cmds = ll_gen_commands(over);
}
