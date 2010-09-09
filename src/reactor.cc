#include "dbic++/reactor.h"

namespace dbi {

    typedef struct ReactorEvent {
        Request *request;
        struct event ev;
    } ReactorEvent;

    void Reactor::initialize() {
        if (_reactorInitialized) return;
        _reactorInitialized = true;
        event_init();
    }

    void Reactor::run() {
        event_dispatch();
    }

    void Reactor::watch(Request *r) {
        ReactorEvent *re = new ReactorEvent;
        re->request = r;
        event_set(&re->ev, r->socket(), EV_READ | EV_WRITE | EV_PERSIST, Reactor::callback, (void*)re);
        event_add(&re->ev, 0);
    }

    void Reactor::callback(int fd, short type, void *arg) {
        ReactorEvent *re = (ReactorEvent *)arg;
        if (re->request->process()) {
            event_del(&re->ev);
            delete re->request;
            delete re;
        }
    }

    void Reactor::stop() {
        event_loopbreak();
    }
}
