#include "visualizer.h"
#include "afd.h"
#include "config.h"
#include "utils.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stddef.h>
#include <string.h>

#define MAX_FEEDING_STR_LEN 256

typedef struct Node {
  char *state;
  Vector2 pos;
  Vector2 vel;
  bool isTarget;
} Node;

// nodes and afd states have the same idxs
DA_new(nodes, Node);

AFD *afd = NULL;

// GLOBALS

bool running = false;

bool show_help = false;
int helpHintTimer = 4; // secs
char *help_info = " \
H            - Show this help \
Enter        - Start typing a new input for the afd\
Ctrl + Enter - Start typing in live mode (feed as you type)\
Esc          - Cancel typing\
Left click   - Drag nodes\
";
bool draggingNode = false;
int mouseOverNode = -1;

bool typingStr = false;
bool liveMode = false;
char feedingStr[MAX_FEEDING_STR_LEN] = "";
bool feeding = false;
int feedingIdx = 0;
int feedingTimer = 0;
bool feedingFinished = false;

const char *msg = NULL;
int msgTimer = 0;

Vector2 mousePos = {0};
Vector2 mouseWorldPos = {0};

Camera2D camera = {0};
Font font;
DA_new(nameLens, float);
float maxR = 0;

void init(AFD *_afd) {
  afd = _afd;
  AFD_reset(afd);

  font = LoadFont("./fonts/CodeSquaredRegular-AYRg.ttf");
  for (int i = 0; i < afd->Q.len; i++) {
    char *state = afd->Q.items[i];
    Node node = {.pos = (Vector2){GetRandomValue(0, 10) * 10,
                                  10 * GetRandomValue(0, 10)},
                 .state = state,
                 .vel = {0},
                 .isTarget = AFD_isStateSuccess(afd, state)

    };
    DA_append(nodes, node);

    DA_append(nameLens, MeasureTextEx(font, state, FONT_SIZE, 0).x);
    maxR = fmaxf(maxR, nameLens.items[nameLens.len - 1]);
  }
  maxR += 2;

  InitWindow(1280, 600, "RAFD");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  SetTargetFPS(100);
  SetExitKey(-1);

  camera = (Camera2D){
      .offset = (Vector2){GetScreenWidth() / 2., GetScreenHeight() / 2.},
      .rotation = 0,
      .target = {0, 0},
      .zoom = 1.};

  running = true;
}

void update(float dt) {
  if (feeding) {
    if (feedingIdx < strlen(feedingStr)) {
      feedingTimer += dt;

      if (feedingTimer >= FEEDING_DELAY) {
        feedingTimer -= FEEDING_DELAY;
        bool validInput = AFD_feedOne(afd, feedingStr[feedingIdx]);

        // abort and notify
        if (validInput) {
          msg = TextFormat("Input %s not in sigma", feedingStr[feedingIdx]);
          msgTimer = 4;
          feeding = false;
        }

        feedingIdx++;
      }
    } else {
      feedingFinished = true;
      feeding = false;
    }
  }

  mousePos = GetMousePosition();
  mouseWorldPos = GetScreenToWorld2D(mousePos, camera);

  if (!draggingNode) {

    mouseOverNode = -1;
    for (int nodeIdx; nodeIdx < nodes.len; nodeIdx++) {
      Node node = nodes.items[nodeIdx];
      char *state = node.state;

      if (CheckCollisionPointCircle(mouseWorldPos, node.pos, maxR)) {
        mouseOverNode = nodeIdx;
        break;
      }
    }
  }
}

void draw() {
  ClearBackground(RAYWHITE);

  BeginMode2D(camera);

  char nextInput = '\0';
  if (feedingIdx < strlen(feedingStr)) {
    nextInput = feedingStr[feedingIdx];
  }

  for (int stateIdx = 0; stateIdx < nodes.len; stateIdx++) {
    Node node = nodes.items[stateIdx];
    char *state = node.state;
    bool isCurrent = AFD_isCurrent(afd, state);
    bool isPrevious = AFD_isPrevious(afd, state);

    // draw edges
    bool selfArrowDrawed = false;
    for (int otherStateIdx = 0; otherStateIdx < nodes.len; otherStateIdx++) {
      Node otherNode = nodes.items[otherStateIdx];
      char *inputs = AFD_getConnection(afd, stateIdx, otherStateIdx);
      int inputsLen = strlen(inputs);

      // skip non connected nodes
      if (inputsLen == 0) {
        continue;
      }

      Color edgeColor = GRAY;

      if (isCurrent) {
        if (feedingIdx < strlen(feedingStr) &&
            !AFD_isValidInput(afd, nextInput)) {
          edgeColor = RED;
        } else if (strchr(inputs, nextInput) !=
                   NULL) { // nextInput is in inputs
          edgeColor = GREEN;
        }
      }

      bool sameNode = strcmp(state, otherNode.state) == 0;

      // if not same state set center as dir between nodes
      if (sameNode && !selfArrowDrawed) {
        // TODO: draw self arrow
        Vector2 centerOffset = {0};
        centerOffset = Vector2Scale(Vector2Normalize(centerOffset), 1.8 * maxR);
      } else {
        // TODO: maybe compute shader?
        Vector2 start, end, dir, dirPerp, controlPoint, endBack, endRight,
            endLeft;
        float padding;

        // draw edge line
        start = node.pos;
        end = otherNode.pos;
        dir = Vector2Subtract(end, start);
        dir = Vector2Normalize(dir);
        padding = maxR + PADDING_FROM_NODE;

        start = Vector2Add(start, Vector2Scale(dir, padding));
        end = Vector2Subtract(end, Vector2Scale(dir, padding));
        controlPoint =
            Vector2Add(Vector2Scale(Vector2Subtract(end, start), .5), start);
        dirPerp = (Vector2){dir.y, -dir.x};
        controlPoint =
            Vector2Add(controlPoint, Vector2Scale(dirPerp, SEP_FACTOR));

        start = Vector2Add(start, Vector2Scale(dirPerp, SEP_FACTOR * .5));
        end = Vector2Add(end, Vector2Scale(dirPerp, SEP_FACTOR * .5));

        Vector2 points[] = {start, controlPoint, end};

        DrawSplineBezierQuadratic(points, 3, THICKNESS, edgeColor);

        dir = Vector2Normalize(Vector2Subtract(end, controlPoint));
        endBack = Vector2Subtract(end, Vector2Scale(dir, TRIANGLE_SIZE));
        endRight =
            Vector2Add(endBack, Vector2Scale(dirPerp, TRIANGLE_SIZE * 0.5));
        endLeft = Vector2Subtract(endBack,
                                  Vector2Scale(dirPerp, TRIANGLE_SIZE * 0.5));

        DrawTriangle(end, endRight, endLeft, edgeColor);
      }
    }
    // for (int inputIdx; inputIdx < afd->sigma.len; inputIdx++) {
    //   char input = afd->sigma.items[inputIdx];
    //
    //   char *nextState = AFD_getNextState(afd, input);
    //   if (nextState && strcmp(nextState, state) == 0) {
    //     Vector2 centerOffset = {0};
    //
    //     for (int otherNodeIdx = 0; otherNodeIdx < nodes.len; otherNodeIdx++)
    //     {
    //       Node otherNode = nodes.items[otherNodeIdx];
    //       if (strcmp(otherNode.state, state) != 0) {
    //         centerOffset = Vector2Add(centerOffset,
    //                                   Vector2Subtract(node.pos,
    //                                   otherNode.pos));
    //       }
    //
    //       centerOffset =
    //           Vector2Scale(Vector2Normalize(centerOffset), 1.8 * maxR);
    //
    //       if (!selfArrowDrawed) {
    //         float angle = RAD2DEG * atan2(centerOffset.y, centerOffset.x);
    //         float start = fmodf((angle + 225 + PADDING_FROM_NODE), 360);
    //         float end = start + 270 - 2 * PADDING_FROM_NODE;
    //
    //         DrawRing(Vector2Add(node.pos, centerOffset), maxR - 2, maxR,
    //         start,
    //                  end, 20, edgeColor);
    //         float endRad = DEG2RAD * (end);
    //         Vector2 arrowEnd, arrowEndBack, arrowBackLeft, arrowBackRight,
    //         dir,
    //             perp;
    //         arrowEnd =
    //             Vector2Add(Vector2Add(node.pos, centerOffset),
    //                        (Vector2){maxR * cosf(endRad), maxR *
    //                        sinf(endRad)});
    //         end -= TRIANGLE_SIZE;
    //         endRad = DEG2RAD * end;
    //         arrowEndBack =
    //             Vector2Add(Vector2Add(node.pos, centerOffset),
    //                        (Vector2){maxR * cosf(endRad), maxR *
    //                        sinf(endRad)});
    //
    //         dir = Vector2Normalize(Vector2Subtract(arrowEnd, arrowEndBack));
    //         perp = (Vector2){-dir.y, dir.x};
    //         arrowBackLeft = Vector2Scale(Vector2Subtract(arrowEndBack, perp),
    //                                      TRIANGLE_SIZE * 0.5);
    //         arrowBackRight = Vector2Scale(Vector2Add(arrowEndBack, perp),
    //                                       TRIANGLE_SIZE * 0.5);
    //
    //         DrawTriangle(arrowEnd, arrowBackLeft, arrowBackRight, edgeColor);
    //       }
    //
    //       Vector2 tagPos =
    //           Vector2Add(node.pos,
    //           Vector2Scale(Vector2Normalize(centerOffset),
    //                                             2.8 * maxR *
    //                                             TAG_SEPARATION));
    //     }
    //   }
    // }

    // draw nodes
    Color nodeColor;
    if (isCurrent && node.isTarget) {
      nodeColor = GREEN;
    } else if (isCurrent) {
      nodeColor = !feedingFinished ? ColorBrightness(RED, 0.6) : RED;
    } else if (isPrevious) {
      nodeColor = ColorBrightness(BROWN, 0.4);
    } else {
      nodeColor = (Color){200, 200, 200, 255};
    }

    Vector2 textPos = Vector2Subtract(
        node.pos,
        Vector2Scale((Vector2){nameLens.items[stateIdx], FONT_SIZE}, 1 / 2.));
    DrawCircleV(node.pos, maxR, nodeColor);
    DrawTextEx(font, state, textPos, FONT_SIZE, 0, BLACK);

    if (node.isTarget) {
      DrawRing(node.pos, maxR * 1.2, maxR * 1.3, 0, 360, 0, GRAY);
    }
  }

  EndMode2D();
};

void input() {

  // quit
  if (!typingStr && IsKeyPressed(KEY_Q))
    running = false;

  // dragging

  if (!draggingNode && mouseOverNode != -1 &&
      IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    draggingNode = true;
  }

  if (draggingNode && mouseOverNode != -1) {
    nodes.items[mouseOverNode].pos = mouseWorldPos;
    if (IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {

      draggingNode = false;
      mouseOverNode = -1;
    }
  }
}

void close() {
  UnloadFont(font);
  DA_free(nodes);
  DA_free(nameLens);
  CloseWindow();
}

void run(AFD *afd) {

  init(afd);
  while (running && !WindowShouldClose()) {
    update(GetFrameTime());
    BeginDrawing();
    draw();
    EndDrawing();
    input();
  }
  close();
}
