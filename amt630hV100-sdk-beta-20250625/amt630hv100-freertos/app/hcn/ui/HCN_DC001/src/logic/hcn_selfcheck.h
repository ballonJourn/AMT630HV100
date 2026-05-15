#ifndef CHECKSELF_H_
#define CHECKSELF_H_

#include <stdbool.h>

typedef enum {
    CHECK_STATE_IDLE,
    CHECK_STATE_WAITING,
    CHECK_STATE_CHECKING,
    CHECK_STATE_FINISHED
}check_state_e;

void selfcheck_init() ;

int  checkself_get_state() ;

void checkself_set_state(check_state_e _state) ;

#endif  // CHECKSELF_H_
