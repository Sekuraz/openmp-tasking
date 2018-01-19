#ifndef TDOMP_ROLES_H
#define TDOMP_ROLES_H

namespace worker {
	void event_loop();
}

namespace re {
	void event_loop();
}

namespace main_thread {
	void event_loop();
}

#endif 
