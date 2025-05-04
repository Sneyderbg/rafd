#define FONT_SIZE 24
#define WIDTH 1280
#define HEIGHT 720
#define FPS 100
#define PHYSICS_FPS 100
#define BACKGROUND_COLOR RAYWHITE
#define CAM_ZOOM_FACTOR 1.2

#define HELP_INFO                                                              \
  "\
H            - Show this help\n\
Enter        - Start typing a new input for the afd\n\
Ctrl + Enter - Start typing in live mode (feed as you type)\n\
Esc          - Cancel typing\n\
Left click   - Drag nodes\n\
Middle click - Move view\n\
\n\
Drag&Drop files to load!\n\
"

#define FONT_PATH "./fonts/CodeSquaredRegular-AYRg.ttf"

// edge style
#define PADDING_FROM_NODE 10
#define SEP_FACTOR 20
#define TRIANGLE_SIZE 20
#define THICKNESS 2
#define EDGE_TAG_SCALE 0.8
#define TAG_SEPARATION 20

// physics
#define EDGE_FORCE 10.0
#define NODE_REPULSION 4e6
#define NODE_DECCEL 8.0
// from when to apply the respective forces in max_r units
#define FORCE_LENGTH_START 4

// feeding
#define FEEDING_DELAY 0.3 // secs
