// ==========================================================================
//  QBIT -- T-Rex Runner (128x64, jump/duck obstacles)
// ==========================================================================
#ifndef TREX_RUNNER_H
#define TREX_RUNNER_H

#include <Arduino.h>

enum class TrexRunnerGestureType {
    None, TouchDown, TouchUp, SingleTap, DoubleTap, LongPress
};

enum class TrexRunnerAction {
    None, Jump, Duck, Exit
};

// Reset state and spawn first obstacle. Call before entering TREX_RUNNING.
void trexRunnerEnter();

// Draw current frame (score, ground, player, obstacles). nowMs is unused (kept for API).
void trexRunnerDrawFrame(unsigned long nowMs = 0);

// Draw game over screen (score + best). Call after entering TREX_OVER.
void trexRunnerDrawGameOver();

// Run one game tick (physics, obstacles, collision). Returns true if game over.
bool trexRunnerTick(unsigned long nowMs);

// Current score (read after trexRunnerTick() returns true for new high score save).
uint32_t trexRunnerGetScore();

// Handle gesture during play. Caller applies Jump/Duck. Exit is not returned during play (exit only by dying).
TrexRunnerAction trexRunnerOnGesture(TrexRunnerGestureType g);

// Apply jump, duck, or release. Caller calls ApplyRelease() on TouchUp, then ApplyJump() if OnGesture returned Jump.
void trexRunnerApplyJump();
void trexRunnerApplyDuck();
void trexRunnerApplyRelease();

#endif // TREX_RUNNER_H
