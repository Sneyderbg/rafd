#include "visualizer.h"
#include "afd.h"
#include "config.h"
#include "utils.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FEEDING_STR_LEN 256

typedef struct Node {
  char *state;
  Vector2 pos;
  Vector2 vel;
  bool isTarget;
} Node;

// nodes && afd states have the same idxs
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
MouseCursor currCursor = MOUSE_CURSOR_DEFAULT;

bool typingStr = false;
bool liveMode = false;
char feedingStr[MAX_FEEDING_STR_LEN] = "";
bool feeding = false;
int feedingIdx = 0;
float feedingTimer = 0;
bool feedingFinished = false;

char msg[1024];
float msgTimer = 0;

Vector2 mousePos = {0};
Vector2 mouseWorldPos = {0};

Camera2D camera = {0};
Font font;
DA_new(nameLens, float);
float maxR = 0;

void init(AFD *_afd) {
  afd = _afd;
  AFD_reset(afd);

  for (int i = 0; i < afd->Q.len; i++) {
    char *state = afd->Q.items[i];
    Node node = {.pos = (Vector2){GetRandomValue(0, 10) * 10,
                                  10 * GetRandomValue(0, 10)},
                 .state = state,
                 .vel = {0},
                 .isTarget = AFD_isStateSuccess(afd, state)

    };
    DA_append(nodes, node);
  }

  InitWindow(1280, 600, "RAFD");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  SetTargetFPS(100);
  SetExitKey(-1);
  font = LoadFont("./fonts/CodeSquaredRegular-AYRg.ttf");

  for (int i = 0; i < afd->Q.len; i++) {
    char *state = afd->Q.items[i];
    DA_append(nameLens, MeasureTextEx(font, state, FONT_SIZE, 0).x);
    maxR = fmaxf(maxR, nameLens.items[nameLens.len - 1]);
  }
  maxR += 2;

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

        // abort && notify
        if (!validInput) {
          strncpy(msg,
                  TextFormat("Input %c not in sigma", feedingStr[feedingIdx]),
                  1024);
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
  if (CheckCollisionPointRec(
          mousePos, (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()})) {
    mouseWorldPos = GetScreenToWorld2D(mousePos, camera);
  }

  if (!draggingNode) {

    mouseOverNode = -1;
    for (int nodeIdx = 0; nodeIdx < nodes.len; nodeIdx++) {
      Node node = nodes.items[nodeIdx];
      char *state = node.state;

      if (CheckCollisionPointCircle(mouseWorldPos, node.pos, maxR)) {
        mouseOverNode = nodeIdx;
        break;
      }
    }
    if (mouseOverNode >= 0 && currCursor == MOUSE_CURSOR_DEFAULT) {
      currCursor = MOUSE_CURSOR_RESIZE_ALL;
      SetMouseCursor(currCursor);
    } else if (mouseOverNode == -1 && currCursor != MOUSE_CURSOR_DEFAULT) {
      currCursor = MOUSE_CURSOR_DEFAULT;
      SetMouseCursor(currCursor);
    }
  }
}

void draw(float dt) {
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
      char *edgeInputs = AFD_getConnection(afd, stateIdx, otherStateIdx);
      int inputsLen = strlen(edgeInputs);

      // skip non connected nodes
      if (inputsLen == 0) {
        continue;
      }

      Color edgeColor = GRAY;

      if (isCurrent) {
        if (feedingIdx < strlen(feedingStr) &&
            !AFD_isValidInput(afd, nextInput)) {
          edgeColor = RED;
        } else if (nextInput != '\0' && strchr(edgeInputs, nextInput) != NULL) {
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

        // TODO: avoid mallocing every frame, but consider the change of afd at
        // runtime
        size_t numInputs = strlen(edgeInputs);
        char *inputsJoined =
            malloc((numInputs + (numInputs - 1)) * sizeof(char));

        for (size_t i = 0; i < numInputs; i++) {
          inputsJoined[2 * i] = edgeInputs[i];
          if (i < numInputs - 1) {
            inputsJoined[2 * i + 1] = ',';
          }
        }

        Vector2 inputsPos =
            Vector2Add(controlPoint, Vector2Scale(dirPerp, TAG_SEPARATION));
        Vector2 inputsSize =
            MeasureTextEx(font, inputsJoined, FONT_SIZE * EDGE_TAG_SCALE, 0);
        DrawTextEx(font, inputsJoined,
                   Vector2Subtract(inputsPos, Vector2Scale(inputsSize, 1 / 2.)),
                   inputsSize.y, 0, edgeColor);

        free(inputsJoined);
      }
    }

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

  // gui

  if (liveMode) {
    Vector2 pos, offset;
    float r, alpha;
    pos = (Vector2){20, 20};
    offset = (Vector2){r * 2, -pos.y / 2};
    r = 8;
    alpha = sinf(GetTime() * 3);
    alpha *= alpha;
    DrawCircleV(pos, r, ColorAlpha(RED, alpha));
    DrawTextEx(font, "Live mode", Vector2Add(pos, offset), FONT_SIZE, 0, BLACK);
  }

  // msg && input
  if (typingStr) {
    const char *textToShow = TextFormat("Enter string: %s", feedingStr);
    if (liveMode) {
      textToShow = "Type your string";
    }

    Vector2 pos =
        (Vector2){FONT_SIZE / 2., GetRenderHeight() - (FONT_SIZE * 1.5)};
    DrawTextEx(font, textToShow, pos, FONT_SIZE, 0, BLACK);
    msg[0] = '\0';

  } else if (msg[0] != '\0' && msgTimer > 0) {
    Vector2 pos =
        (Vector2){FONT_SIZE / 2., GetRenderHeight() - FONT_SIZE * 1.5};
    msgTimer -= dt;
    DrawTextEx(font, TextFormat("Error: ", msg), pos, FONT_SIZE, 0,
               (Color){200, 0, 0, fminf(255, 255 * msgTimer)});
  }

  // draw feeding progress
  if (feeding || (liveMode && strlen(feedingStr) > 0)) {
    // TODO: measure feedingStr[: feedingIdx + 1] for offset, i will assume the
    // font is always monospace
    int baseCharWidth, strCursorAfter, strCursorOffset, strWidth, strPos, r;
    Color c;
    baseCharWidth = MeasureTextEx(font, "a", FONT_SIZE, 0).x;
    strCursorOffset = baseCharWidth * feedingIdx + baseCharWidth / 2.;
    strWidth = MeasureTextEx(font, feedingStr, FONT_SIZE, 0).x;
    strPos = (GetRenderWidth() - strWidth) / 2.;
    DrawTextEx(font, feedingStr, (Vector2){strPos, 20}, FONT_SIZE, 0, GRAY);
    r = 4;
    c = (feedingIdx < strlen(feedingStr) &&
                 AFD_isValidInput(afd, feedingStr[feedingIdx])
             ? GREEN
             : RED);
    DrawCircleV((Vector2){strPos + strCursorOffset, 20 + FONT_SIZE + r * 2}, r,
                c);
  }

  else if (feedingFinished) {
    const char *finishedMsg = "Feed finished";
    int msgW = MeasureTextEx(font, finishedMsg, FONT_SIZE, 0).x;
    DrawTextEx(font, finishedMsg, (Vector2){(GetRenderWidth() - msgW) / 2., 20},
               FONT_SIZE, 0, BLACK);
  }
};

void stopFeeding() {
  feeding = false;
  feedingIdx = 0;
  feedingFinished = false;
}

void startFeeding() {
  feeding = true;
  feedingFinished = false;
  feedingIdx = 0;
}

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

  // typing
  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {

    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
      liveMode = true;
    }

    if (!typingStr) {
      AFD_reset(afd);

      if (!liveMode)
        stopFeeding();

      typingStr = true;

    } else {

      if (!liveMode)
        startFeeding();
      else if (strlen(feedingStr) > 0) {

        // TODO: use feedingIdx instead of strlen
        AFD_feedOne(afd, feedingStr[strlen(feedingStr) - 1]);
        memset(feedingStr, 0, MAX_FEEDING_STR_LEN);
      }
      typingStr = false;
      liveMode = false;
    }
  }

  if (IsKeyPressed(KEY_ESCAPE)) {
    if (typingStr) {
      memset(feedingStr, 0, MAX_FEEDING_STR_LEN);
      typingStr = false;
      liveMode = false;
    } else
      stopFeeding();
  }

  if (typingStr) {
    char key = GetCharPressed();
    while (key) {
      if (32 <= key && key <= 126) { // printable chars
        feedingStr[strlen(feedingStr)] = key;
        if (liveMode) {
          if (strlen(feedingStr) > 1) {

            bool succ = AFD_feedOne(afd, feedingStr[strlen(feedingStr) - 2]);
          }
          feedingIdx = strlen(feedingStr) - 1;
        }
        key = GetCharPressed();
      }
    }
  }

  if (!liveMode && IsKeyPressed(KEY_BACKSPACE)) {
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
      memset(feedingStr, 0, MAX_FEEDING_STR_LEN);
    } else {
      feedingStr[strlen(feedingStr) - 1] = '\0';
    }
  }

  if (typingStr && !liveMode && IsKeyDown(KEY_LEFT_CONTROL) &&
      IsKeyPressed(KEY_V)) {
    strncpy(feedingStr, GetClipboardText(), MAX_FEEDING_STR_LEN);
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
    float dt = GetFrameTime();
    update(dt);
    BeginDrawing();
    draw(dt);
    EndDrawing();
    input();
  }
  close();
}
