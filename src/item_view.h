#ifndef ITEM_VIEW_H
#define ITEM_VIEW_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/texture2d.hpp>

namespace godot {

class ItemView final : public RefCounted {
	GDCLASS(ItemView, RefCounted)

	int _id = 0;
	Ref<Texture2D> _icon;
	int _item_amount = 1;
	String _name;
	String _category;
	String _hint_description;

protected:
	static void _bind_methods();

public:
	static constexpr int MAX_STACK = 99;
	int get_id() const { return _id; }
	void set_id(int value) { _id = value; }
	Ref<Texture2D> get_icon() const { return _icon; }
	void set_icon(const Ref<Texture2D> &value) { _icon = value; }
	int get_item_amount() const { return _item_amount; }
	void set_item_amount(int value) { _item_amount = value < 1 ? 1 : value > MAX_STACK ? MAX_STACK : value; }
	String get_name() const { return _name; }
	void set_name(const String &value) { _name = value; }
	String get_category() const { return _category; }
	void set_category(const String &value) { _category = value; }
	String get_hint_description() const { return _hint_description; }
	void set_hint_description(const String &value) { _hint_description = value; }
	Ref<ItemView> duplicate_item() const;
};

} // namespace godot

#endif // ITEM_VIEW_H
