#pragma once

#include "afd.h"

void init(AFD *_afd);
void update(float dt);
void draw(float dt);
void input();
void close();
void run(AFD *afd);
