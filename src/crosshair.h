

#ifndef CROSSHAIR_H
#define CROSSHAIR_H

#include <godot_cpp/classes/control.hpp>

namespace godot {

class Crosshair : public Control {
	GDCLASS(Crosshair, Control)

protected:
	static void _bind_methods();

public:
	void _draw() override;
};

}

#endif //CROSSHAIR_H
