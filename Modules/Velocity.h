#pragma once
#include <Windows.h>
#include <vector>
#include <thread>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <random>
#define MINECRAFT_TICK 600
#define MINECRAFT_HANDLE checkMinecraftHandle()

#pragma comment(lib,"winmm.lib")

void VelocityScanner();

void changeVelocity(int min_h_val, int max_h_val, int min_v_val, int max_v_val, int chance, bool negative, bool invert, bool while_sprinting);