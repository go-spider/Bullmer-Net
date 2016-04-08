#include "../Interface/xTypes.h"
#include "xBufferEx.h"

static u32_t const ptr_size   = sizeof(x_node_t*);
static u32_t const ptr_sizex2 = 2 * sizeof(x_node_t*);
static u32_t const block_size = sizeof(x_block_node_t);