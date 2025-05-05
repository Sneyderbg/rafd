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
#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif /* ifdef PLATFORM_WEB */

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

int INITIAL_WIDTH = WIDTH;
int INITIAL_HEIGHT = HEIGHT;
bool running = false;

bool showHelp = false;
float helpHintTimer = 4; // secs
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

char infoMsg[1024];
char errorMsg[1024];
float msgTimer = 0;

Vector2 mousePos = {0};
Vector2 prevMousePos = {0};
Vector2 mouseWorldPos = {0};

Camera2D camera = {0};
Font font;
Shader bgShader;
Texture2D bgTexture;

DA_new(nameLens, float);
float maxR = 0;
DA_new(inputsJoined, char);

bool loadFileDef(char *filename) {
  char *error;
  AFD *newAfd = AFD_parse(filename, ' ', &error);

  if (newAfd == NULL) {
    strcpy(errorMsg, error);
    msgTimer = 6;
    return false;
  }

  if (afd != NULL) {
    AFD_free(afd);
  }
  afd = newAfd;
  AFD_reset(afd);

  // initialize nodes and state name lens
  nodes.len = 0;
  nameLens.len = 0;
  for (size_t i = 0; i < afd->Q.len; i++) {
    char *state = afd->Q.items[i];
    Node node = {.pos = (Vector2){GetRandomValue(0, 10) * 10,
                                  10 * GetRandomValue(0, 10)},
                 .state = state,
                 .vel = {0},
                 .isTarget = AFD_isStateSuccess(afd, state)

    };
    DA_append(nodes, node);

    // measure state names
    DA_append(nameLens, MeasureTextEx(font, state, FONT_SIZE, 0).x);
    maxR = fmaxf(maxR, nameLens.items[nameLens.len - 1]);
  }
  maxR += 2;

  return true;
}

void strJoin(char *chars, char sep) {
  size_t n = strlen(chars);
  inputsJoined.len = 0;
  for (size_t i = 0; i < n; i++) {
    DA_append(inputsJoined, chars[i]);
    if (i < n - 1) {
      DA_append(inputsJoined, sep);
    } else {
      DA_append(inputsJoined, '\0');
    }
  }
}

void init() {
#ifdef PLATFORM_WEB
  emscripten_get_screen_size(&INITIAL_WIDTH, &INITIAL_HEIGHT);
  InitWindow(INITIAL_WIDTH, INITIAL_HEIGHT, "RAFD");
#else
  InitWindow(WIDTH, HEIGHT, "RAFD");
#endif /* ifdef PLATFORM_WEB */
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  SetExitKey(-1);
  font = LoadFont("./fonts/CodeSquaredRegular-AYRg.ttf");
#ifdef PLATFORM_WEB
  bgShader = LoadShader(NULL, "./shaders/bgweb.frag");
#else
  bgShader = LoadShader(NULL, "./shaders/bg.frag");
#endif /* ifdef PLATFORM_WEB */
  Image bgIm = GenImageColor(WIDTH, HEIGHT, RAYWHITE);
  bgTexture = LoadTextureFromImage(bgIm);
  UnloadImage(bgIm);

  camera = (Camera2D){
      .offset = (Vector2){GetScreenWidth() / 2., GetScreenHeight() / 2.},
      .rotation = 0,
      .target = {0, 0},
      .zoom = 1.};

  bool loaded = loadFileDef("def.afdd");
  if (!loaded) {
    msgTimer = 6;
  }

  running = true;
}

void updatePhysics(float dt) {

  if (afd == NULL) {
    return;
  }

  for (size_t nodeIdx = 0; nodeIdx < nodes.len; nodeIdx++) {
    Node *node = &nodes.items[nodeIdx];
    for (size_t otherNodeIdx = nodeIdx + 1; otherNodeIdx < nodes.len;
         otherNodeIdx++) {

      Node *otherNode = &nodes.items[otherNodeIdx];
      Vector2 dir, attractionF = {0}, repelF, finalF;
      float distance, edgeLen;

      dir = Vector2Subtract(otherNode->pos, node->pos);
      distance = Vector2Length(dir);
      edgeLen = fmaxf(0, distance - maxR * (2 + FORCE_LENGTH_START));
      distance = fmaxf(maxR * 2, distance - maxR * (2 + FORCE_LENGTH_START));
      dir = Vector2Normalize(dir);

      // if there is a connection in any direction attract both nodes
      if (AFD_hasConnection(afd, nodeIdx, otherNodeIdx) ||
          AFD_hasConnection(afd, otherNodeIdx, nodeIdx)) {
        attractionF = Vector2Scale(dir, EDGE_FORCE * edgeLen);
      }

      // repel each pair of nodes
      repelF = Vector2Scale(dir, -NODE_REPULSION / (distance * distance));

      finalF = Vector2Add(attractionF, repelF);

      node->vel = Vector2Add(node->vel, Vector2Scale(finalF, dt));
      otherNode->vel =
          Vector2Subtract(otherNode->vel, Vector2Scale(finalF, dt));
    }
  }

  // apply velocities
  for (size_t nodeIdx = 0; nodeIdx < nodes.len; nodeIdx++) {
    Node *node = &nodes.items[nodeIdx];

    // deccelerate nodes so they reach an stable positioning
    Vector2 deccelF = Vector2Scale(Vector2Normalize(node->vel), NODE_DECCEL);
    node->vel = Vector2Subtract(node->vel, deccelF);

    node->pos = Vector2Add(node->pos, Vector2Scale(node->vel, dt));
  }
}

void update(float dt) {
  if (afd != NULL && feeding) {
    if (feedingIdx < (int)strlen(feedingStr)) {
      feedingTimer += dt;

      if (feedingTimer >= FEEDING_DELAY) {
        feedingTimer -= FEEDING_DELAY;
        bool validInput = AFD_feedOne(afd, feedingStr[feedingIdx]);

        // abort && notify
        if (!validInput) {
          strncpy(errorMsg,
                  TextFormat("Input %c not in sigma", feedingStr[feedingIdx]),
                  1024);
          msgTimer = 4;
          feeding = false;
        } else {
          feedingIdx++;
        }
      }
    } else {
      feedingFinished = true;
      feeding = false;
    }
  }

  prevMousePos = mousePos;
#ifdef PLATFORM_WEB
  // calculate mousePos based on internal size and actual canvas size
  mousePos = Vector2Multiply(
      GetMousePosition(), (Vector2){1. * GetRenderWidth() / INITIAL_WIDTH,
                                    1. * GetRenderHeight() / INITIAL_HEIGHT});
#else
  mousePos = GetMousePosition();
#endif /* ifdef PLATFORM_WEB */

  if (CheckCollisionPointRec(
          mousePos, (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()})) {
    mouseWorldPos = GetScreenToWorld2D(mousePos, camera);
  }

  if (!draggingNode) {

    mouseOverNode = -1;
    for (size_t nodeIdx = 0; nodeIdx < nodes.len; nodeIdx++) {
      Node node = nodes.items[nodeIdx];

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

  if (IsFileDropped()) {
    FilePathList files = LoadDroppedFiles();
    if (files.count != 1) {
      strcpy(errorMsg, "Only one dropped file is supported");
      msgTimer = 4;
    } else {
      if (loadFileDef(files.paths[0])) {
        strcpy(infoMsg, "File definition loaded!");
        msgTimer = 4;
      }
    }
    UnloadDroppedFiles(files);
  }

  if (IsWindowResized()) {
    camera.offset = (Vector2){GetScreenWidth() / 2., GetScreenHeight() / 2.};
  }
}

void draw(float dt) {
  ClearBackground(RAYWHITE);

  if (IsShaderValid(bgShader)) {

    const int resLoc = GetShaderLocation(bgShader, "resolution");
    const int targetLoc = GetShaderLocation(bgShader, "camTarget");
    const int zoomLoc = GetShaderLocation(bgShader, "camZoom");

    SetShaderValue(bgShader, resLoc,
                   &(Vector2){GetRenderWidth(), GetRenderHeight()},
                   SHADER_UNIFORM_VEC2);
    SetShaderValue(bgShader, targetLoc,
                   &(Vector2){camera.target.x, -camera.target.y},
                   SHADER_UNIFORM_VEC2);
    SetShaderValue(bgShader, zoomLoc, &camera.zoom, SHADER_UNIFORM_FLOAT);

    BeginShaderMode(bgShader);
    DrawTexturePro(bgTexture, (Rectangle){0, 0, WIDTH, HEIGHT},
                   (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()},
                   (Vector2){0, 0}, 0, WHITE);
    EndShaderMode();
  }

  if (afd != NULL) {

    BeginMode2D(camera);

    char nextInput = '\0';
    if (feedingIdx < (int)strlen(feedingStr)) {
      nextInput = feedingStr[feedingIdx];
    }

    for (size_t stateIdx = 0; stateIdx < nodes.len; stateIdx++) {
      Node node = nodes.items[stateIdx];
      char *state = node.state;
      bool isCurrent = AFD_isCurrent(afd, state);
      bool isPrevious = AFD_isPrevious(afd, state);
      bool isNext = AFD_isNext(afd, state, nextInput);

      // draw edges
      for (size_t otherStateIdx = 0; otherStateIdx < nodes.len;
           otherStateIdx++) {
        Node otherNode = nodes.items[otherStateIdx];
        char *edgeInputs = AFD_getConnection(afd, stateIdx, otherStateIdx);
        int inputsLen = strlen(edgeInputs);

        // skip non connected nodes
        if (inputsLen == 0) {
          continue;
        }

        Color edgeColor = GRAY;

        if (isCurrent) {
          if (feedingIdx < (int)strlen(feedingStr) &&
              !AFD_isValidInput(afd, nextInput)) {
            edgeColor = RED;
          } else if (nextInput != '\0' &&
                     strchr(edgeInputs, nextInput) != NULL) {
            edgeColor = GREEN;
          }
        }

        bool sameNode = strcmp(state, otherNode.state) == 0;

        // draw self arrow
        if (sameNode) {
          Vector2 centerOffset = {0}, diff, arrowEnd, arrowEndBack, dir, perp;
          float angle, start, end, endRad;

          // accumulate inverted directions to other nodes
          for (size_t i = 0; i < nodes.len; i++) {
            if (stateIdx != i) {
              diff = Vector2Subtract(node.pos, nodes.items[i].pos);
              centerOffset = Vector2Add(centerOffset, diff);
            }
          }

          centerOffset =
              Vector2Scale(Vector2Normalize(centerOffset), 1.8 * maxR);

          angle = atan2f(centerOffset.y, centerOffset.x) * RAD2DEG;
          start = fmodf((angle + 225 + PADDING_FROM_NODE), 360);
          end = start + 270 - 2 * PADDING_FROM_NODE;

          DrawRing(Vector2Add(node.pos, centerOffset), maxR - 2, maxR, start,
                   end, 20, edgeColor);
          endRad = end * DEG2RAD;

          arrowEnd = Vector2Add(node.pos, centerOffset);
          arrowEnd = Vector2Add(
              arrowEnd, (Vector2){maxR * cosf(endRad), maxR * sinf(endRad)});

          end -= TRIANGLE_SIZE;
          endRad = (end)*DEG2RAD;

          arrowEndBack = Vector2Add(node.pos, centerOffset);
          arrowEndBack =
              Vector2Add(arrowEndBack,
                         (Vector2){maxR * cosf(endRad), maxR * sinf(endRad)});

          dir = Vector2Normalize(Vector2Subtract(arrowEnd, arrowEndBack));
          perp = Vector2Scale((Vector2){-dir.y, dir.x}, TRIANGLE_SIZE * .5);
          DrawTriangle(arrowEnd, Vector2Subtract(arrowEndBack, perp),
                       Vector2Add(arrowEndBack, perp), edgeColor);

          strJoin(edgeInputs, ',');

          Vector2 inputsPos =
              Vector2Add(node.pos, Vector2Scale(Vector2Normalize(centerOffset),
                                                2.8 * maxR + TAG_SEPARATION));
          Vector2 inputsSize = MeasureTextEx(font, inputsJoined.items,
                                             FONT_SIZE * EDGE_TAG_SCALE, 0);
          DrawTextEx(
              font, inputsJoined.items,
              Vector2Subtract(inputsPos, Vector2Scale(inputsSize, 1 / 2.)),
              inputsSize.y, 0, edgeColor);

        } else { // draw other arrows

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

          strJoin(edgeInputs, ',');

          Vector2 inputsPos =
              Vector2Add(controlPoint, Vector2Scale(dirPerp, TAG_SEPARATION));
          Vector2 inputsSize = MeasureTextEx(font, inputsJoined.items,
                                             FONT_SIZE * EDGE_TAG_SCALE, 0);
          DrawTextEx(
              font, inputsJoined.items,
              Vector2Subtract(inputsPos, Vector2Scale(inputsSize, 1 / 2.)),
              inputsSize.y, 0, edgeColor);
        }
      }

      // draw nodes
      Color nodeColor;
      if (isCurrent && node.isTarget) {
        nodeColor = GREEN;
      } else if (isCurrent) {
        nodeColor = !feedingFinished ? ColorBrightness(RED, 0.6) : RED;
      } else if (isPrevious) {
        nodeColor = ColorBrightness(BLACK, 0.5);
      } else {
        nodeColor = (Color){200, 200, 200, 255};
      }

      Vector2 textPos = Vector2Subtract(
          node.pos,
          Vector2Scale((Vector2){nameLens.items[stateIdx], FONT_SIZE}, 1 / 2.));
      DrawCircleV(node.pos, maxR, nodeColor);
      if (isNext) {
        DrawRing(node.pos, maxR - 4, maxR - 2, 0, 360, 20, GREEN);
      }
      DrawTextEx(font, state, textPos, FONT_SIZE, 0, BLACK);

      if (node.isTarget) {
        DrawRing(node.pos, maxR * 1.2, maxR * 1.3, 0, 360, 0, GRAY);
      }
    }

    EndMode2D();
  }

  // gui

  if (liveMode) {
    Vector2 pos, offset;
    float r, alpha;
    pos = (Vector2){20, 20};
    r = 8;
    alpha = sinf(GetTime() * 3);
    alpha *= alpha;
    offset = (Vector2){r * 2, -pos.y / 2};
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
    errorMsg[0] = '\0';

  } else if (msgTimer > 0) {

    Vector2 pos =
        (Vector2){FONT_SIZE / 2., GetRenderHeight() - FONT_SIZE * 1.5};
    if (errorMsg[0] != '\0') {
      DrawTextEx(font, TextFormat("Error: %s", errorMsg), pos, FONT_SIZE, 0,
                 (Color){200, 0, 0, fminf(255, 255 * msgTimer)});
      pos.y -= FONT_SIZE + 2;
    }
    if (infoMsg[0] != '\0') {
      DrawTextEx(font, infoMsg, pos, FONT_SIZE, 0,
                 (Color){0, 100, 200, fminf(255, 255 * msgTimer)});
    }
    msgTimer -= dt;
  }

  // draw feeding progress
  if (afd != NULL && (feeding || (liveMode && strlen(feedingStr) > 0))) {
    int cursorBeforeFeed, cursorAfterFeed, strCursorOffset, strWidth, strPos, r;
    Color c;

    char feedingStrCpy[MAX_FEEDING_STR_LEN];
    strncpy(feedingStrCpy, feedingStr, feedingIdx + 1);

    feedingStrCpy[feedingIdx + 1] = '\0';
    cursorAfterFeed = MeasureTextEx(font, feedingStrCpy, FONT_SIZE, 0).x;

    feedingStrCpy[feedingIdx] = '\0';
    cursorBeforeFeed = MeasureTextEx(font, feedingStrCpy, FONT_SIZE, 0).x;

    strCursorOffset = (cursorBeforeFeed + cursorAfterFeed) / 2.;
    strWidth = MeasureTextEx(font, feedingStr, FONT_SIZE, 0).x;
    strPos = (GetRenderWidth() - strWidth) / 2.;
    DrawTextEx(font, feedingStr, (Vector2){strPos, 20}, FONT_SIZE, 0, GRAY);
    r = 4;
    c = (feedingIdx < (int)strlen(feedingStr) &&
                 AFD_isValidInput(afd, feedingStr[feedingIdx])
             ? GREEN
             : RED);
    DrawCircleV((Vector2){strPos + strCursorOffset, 20 + FONT_SIZE + r * 2}, r,
                c);
  } else if (feedingFinished) {
    const char *finishedMsg = "Feed finished";
    int msgW = MeasureTextEx(font, finishedMsg, FONT_SIZE, 0).x;
    DrawTextEx(font, finishedMsg, (Vector2){(GetRenderWidth() - msgW) / 2., 20},
               FONT_SIZE, 0, BLACK);
  }

  if (helpHintTimer > 0) {

    helpHintTimer -= dt;
    const char *hint = "Hold H to show help";
    DrawTextEx(
        font, hint,
        (Vector2){
            (GetRenderWidth() - MeasureTextEx(font, hint, FONT_SIZE, 0).x) / 2,
            80},
        FONT_SIZE, 0, ColorAlpha(BLACK, helpHintTimer));
  }

  if (showHelp) {
    const float scaledSize = 1. * FONT_SIZE * GetScreenWidth() / INITIAL_WIDTH;
    DrawRectangleRec((Rectangle){0, 0, GetRenderWidth(), GetRenderHeight()},
                     (Color){0, 0, 0, 128});
    Vector2 helpSize = MeasureTextEx(font, HELP_INFO, scaledSize, 1);
    DrawTextEx(font, HELP_INFO,
               (Vector2){(GetRenderWidth() - helpSize.x) / 2,
                         (GetRenderHeight() - helpSize.y) / 2},
               scaledSize, 0, BLACK);
  }
}

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

#ifdef DEBUG
void drawDebug() { DrawFPS(10, 10); }
#endif

void input() {

#ifndef PLATFORM_WEB
  // reload shaders
  if (IsKeyPressed(KEY_R)) {
    UnloadShader(bgShader);
    bgShader = LoadShader(NULL, "./shaders/bg.frag");
  }
#endif

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
    nodes.items[mouseOverNode].vel = Vector2Zero();
    if (IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {

      draggingNode = false;
      mouseOverNode = -1;
    }
  }

  int k = GetKeyPressed();
  if (k != 0) {
    printf("key pressed: %d\n", k);
  }

  // move camera
  if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
    camera.target = Vector2Subtract(
        camera.target, Vector2Scale(Vector2Subtract(mousePos, prevMousePos),
                                    1.0 / camera.zoom));
  }

  // zoom
  const float wheel = GetMouseWheelMove();
  if (wheel > 0) {
    camera.zoom *= CAM_ZOOM_FACTOR;
  } else if (wheel < 0) {
    camera.zoom /= CAM_ZOOM_FACTOR;
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

            AFD_feedOne(afd, feedingStr[strlen(feedingStr) - 2]);
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

  if (IsKeyPressed(KEY_H)) {
    showHelp = true;
  }
  if (IsKeyReleased(KEY_H)) {
    showHelp = false;
  }
}

void closeVis() {
  if (afd != NULL) {
    AFD_free(afd);
  }
  UnloadTexture(bgTexture);
  UnloadShader(bgShader);
  UnloadFont(font);
  DA_free(inputsJoined);
  DA_free(nodes);
  DA_free(nameLens);
  CloseWindow();
}

float fixedDt = 1.0 / PHYSICS_FPS;
float dtAcc = 0.;
void loop() {
  float dt = GetFrameTime();
  dtAcc += fixedDt;

  while (dtAcc >= fixedDt) {
    updatePhysics(fixedDt);
    dtAcc -= fixedDt;
  }

  input();
  update(dt);
  BeginDrawing();
  draw(dt);
#ifdef DEBUG
  drawDebug();
#endif
  EndDrawing();
}

void runVis() {
  init();

#ifdef PLATFORM_WEB
  emscripten_set_main_loop(loop, 0, false);
#else
  SetTargetFPS(FPS);
  while (running && !WindowShouldClose()) {
    loop();
  }
  closeVis();
#endif /* ifdef WEB */
}
