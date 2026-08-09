#ifndef TMUX_H
#define TMUX_H

#include <stdbool.h>
#include "../DAP/target.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize TMUX select GPIOs and deselect all targets. */
void TMUX_Init(void);

bool TMUX_SelectNone(void);
bool TMUX_SelectG474(void);
bool TMUX_SelectF429(void);

/** Select a TMUX route for a Target Manager target value. */
bool TMUX_Select(target_t target);

#ifdef __cplusplus
}
#endif

#endif /* TMUX_H */
