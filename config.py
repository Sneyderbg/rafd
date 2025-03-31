import pyray as pr

FONT_SIZE = 24
SIZE = (1280, 720)
FPS = 100
PHYSICS_FPS = 60
BACKGROUND_COLOR = pr.RAYWHITE

FONT_PATH = "./fonts/CodeSquaredRegular-AYRg.ttf"

# edge style
PADDING_FROM_NODE = 10
SEP_FACTOR = 20
TRIANGLE_SIZE = 20
THICKNESS = 2
EDGE_TAG_SCALE = 0.6
TAG_SEPARATION = 20

# physics
EDGE_FORCE = 5.0
NODE_REPULSION = 2e6
NODE_DECCEL = 4.0
FORCE_LENGTH_START = 4
"from when to apply the respective forces in max_r units"
