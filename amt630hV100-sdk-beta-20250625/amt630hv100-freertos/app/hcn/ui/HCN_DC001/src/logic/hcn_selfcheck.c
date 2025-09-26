#include "hcn_selfcheck.h"
#include "awtk.h"
#include "hcn_logic.h"

#define SELF_CHECK_DURATION 2000 
#define SELF_CHECK_INTERVAL 50

typedef struct {
    uint32_t timer_id;
    uint32_t start_tick ;
    uint16_t check_count;
    check_state_e state;

}checkself_manager_t ;

static checkself_manager_t manager = { 0 };

static int max_check_count = 30;
static double half_check_count;
static double rpm_div;
static double speed_div;

static void handle_state_idle() {
  // TODO: Implement idle state logic if needed
}

static void handle_state_waiting() {
    uint32_t elapsed = time_now_ms() - manager.start_tick;

    if (elapsed > 1000 ) 
    {     //动画播放完毕
        manager.state      = CHECK_STATE_CHECKING;
        manager.start_tick = time_now_ms();
    }

    return ; 
}

static void handle_state_checking() {
    manager.check_count++;

    if (manager.check_count > max_check_count) {
        manager.state = CHECK_STATE_FINISHED;
        return ;
    }

    int count = (manager.check_count > half_check_count)
                    ? (max_check_count - manager.check_count)
                    : manager.check_count;

    int rpm_value   = (int)(count * rpm_div);
    int speed_value = (int)(count * speed_div);

    // printf(" handle_state_checking rpm_value = %d speed_value = %d\n" , rpm_value , speed_value);
    home_refresh_rpm(rpm_value)  ;
    home_refresh_speed(speed_value);

    return ;
}

static void handle_state_finished() {
    if (manager.timer_id) {
        timer_remove(manager.timer_id);
        manager.timer_id = 0;
    }

    home_refresh_signal_visible(FALSE);

    manager.state       = CHECK_STATE_FINISHED ;

    return ;
}

static ret_t on_handle_timer(const timer_info_t *info) {
    (void)info ;
    switch (manager.state) 
    {
        case CHECK_STATE_IDLE:
            handle_state_idle();
            break;
        case CHECK_STATE_WAITING:
            handle_state_waiting();
            break;
        case CHECK_STATE_CHECKING:
            handle_state_checking();
            break;
        case CHECK_STATE_FINISHED:
            handle_state_finished();
            break;
        default:
            printf("Unknown state in checkself manager");
            break;
    }

    return RET_REPEAT ;
}


void selfcheck_init() {
    manager.state       = CHECK_STATE_WAITING;
    manager.start_tick  = time_now_ms() ;
    manager.check_count = 0 ;
    max_check_count     = SELF_CHECK_DURATION / SELF_CHECK_INTERVAL ;
    half_check_count    = max_check_count / 2.0 ;

    rpm_div             = (double)(RPM_MAX )   / half_check_count;
    speed_div           = (double)(SPEED_MAX) / half_check_count;
    

    home_refresh_signal_visible(TRUE);

    manager.timer_id    = timer_add(on_handle_timer, NULL, SELF_CHECK_INTERVAL);
    if (manager.timer_id == 0) {
        printf("Failed to add timer for checkself manager \n");
        return;
    }
}

int checkself_get_state() {
    return manager.state ;
}

void checkself_set_state(check_state_e _state) {
    manager.state = _state ;
}
