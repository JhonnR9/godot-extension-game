#include "item_view.h"

namespace godot {

void ItemView::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_id"), &ItemView::get_id);
	ClassDB::bind_method(D_METHOD("set_id", "id"), &ItemView::set_id);
	ClassDB::bind_method(D_METHOD("get_icon"), &ItemView::get_icon);
	ClassDB::bind_method(D_METHOD("set_icon", "icon"), &ItemView::set_icon);
	ClassDB::bind_method(D_METHOD("get_item_amount"), &ItemView::get_item_amount);
	ClassDB::bind_method(D_METHOD("set_item_amount", "amount"), &ItemView::set_item_amount);
	ClassDB::bind_method(D_METHOD("get_name"), &ItemView::get_name);
	ClassDB::bind_method(D_METHOD("set_name", "name"), &ItemView::set_name);
	ClassDB::bind_method(D_METHOD("get_category"), &ItemView::get_category);
	ClassDB::bind_method(D_METHOD("set_category", "category"), &ItemView::set_category);
	ClassDB::bind_method(D_METHOD("get_hint_description"), &ItemView::get_hint_description);
	ClassDB::bind_method(D_METHOD("set_hint_description", "description"), &ItemView::set_hint_description);
	ClassDB::bind_method(D_METHOD("duplicate_item"), &ItemView::duplicate_item);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "id"), "set_id", "get_id");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "icon", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_icon", "get_icon");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "item_amount"), "set_item_amount", "get_item_amount");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "set_name", "get_name");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "category"), "set_category", "get_category");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "hint_description"), "set_hint_description", "get_hint_description");
}

Ref<ItemView> ItemView::duplicate_item() const {
	Ref<ItemView> copy;
	copy.instantiate();
	copy->set_id(_id);
	copy->set_icon(_icon);
	copy->set_item_amount(_item_amount);
	copy->set_name(_name);
	copy->set_category(_category);
	copy->set_hint_description(_hint_description);
	return copy;
}

} // namespace godot
