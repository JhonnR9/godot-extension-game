#include "grid_inventory.h"

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/label_settings.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

int64_t GridInventory::_make_key(const int column, const int row) const {
	const uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(column)) << 32) |
			static_cast<uint32_t>(row);
	return static_cast<int64_t>(key);
}

int64_t GridInventory::_key_at_position(const Point2i &position) const {
	const Point2i adjusted = position - _grid_padding;
	const int cell_width = _slot_size.x + _slot_margin.x;
	const int cell_height = _slot_size.y + _slot_margin.y;
	if (adjusted.x < 0 || adjusted.y < 0 || cell_width <= 0 || cell_height <= 0) return INVALID_KEY;
	const int column = adjusted.x / cell_width;
	const int row = adjusted.y / cell_height;
	const int64_t key = _make_key(column, row);
	const Slot *slot = _cells.getptr(key);
	return slot && slot->rect.has_point(position) ? key : INVALID_KEY;
}

void GridInventory::_draw_background() {
	if (_background.is_valid()) _background->draw(get_canvas_item(), Rect2(Point2(), get_size()));
}

Panel *GridInventory::_create_slot_panel(const Point2i &position) {
	Panel *panel = memnew(Panel);
	panel->set_position(position);
	panel->set_size(_slot_size);
	panel->set_mouse_filter(_interaction_enabled ? MOUSE_FILTER_STOP : MOUSE_FILTER_IGNORE);
	panel->connect("mouse_entered", callable_mp(this, &GridInventory::_on_slot_mouse_entered));
	panel->connect("mouse_exited", callable_mp(this, &GridInventory::_on_slot_mouse_exited));
	panel->connect("gui_input", callable_mp(this, &GridInventory::_on_slot_gui_input));
	panel->set_drag_forwarding(
			callable_mp(this, &GridInventory::_make_drag_data),
			callable_mp(this, &GridInventory::_accept_drop_data),
			callable_mp(this, &GridInventory::_handle_drop_data));
	return panel;
}

Label *GridInventory::_create_count_label() {
	Label *label = memnew(Label);
	label->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	label->set_vertical_alignment(VERTICAL_ALIGNMENT_BOTTOM);
	label->set_offset(SIDE_RIGHT, -3.0);
	label->set_offset(SIDE_BOTTOM, -1.0);
	label->set_mouse_filter(MOUSE_FILTER_IGNORE);
	if (_count_label_settings.is_valid()) label->set_label_settings(_count_label_settings);
	return label;
}

void GridInventory::_clear_slot(Slot &slot) {
	if (slot.icon) {
		slot.icon->queue_free();
		slot.icon = nullptr;
	}
	slot.item.unref();
	if (slot.count_label) slot.count_label->set_text(String());
	if (slot.panel) slot.panel->set_tooltip_text(String());
}

void GridInventory::_sync_slot(Slot &slot) {
	if (slot.item.is_null()) {
		if (slot.icon) {
			slot.icon->queue_free();
			slot.icon = nullptr;
		}
		if (slot.count_label) slot.count_label->set_text(String());
		if (slot.panel) slot.panel->set_tooltip_text(String());
		return;
	}
	if (slot.item->get_icon().is_valid()) {
		if (!slot.icon) {
			slot.icon = memnew(TextureRect);
			slot.icon->set_mouse_filter(MOUSE_FILTER_IGNORE);
			slot.icon->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
			slot.icon->set_offset(SIDE_LEFT, 4.0);
			slot.icon->set_offset(SIDE_TOP, 4.0);
			slot.icon->set_offset(SIDE_RIGHT, -4.0);
			slot.icon->set_offset(SIDE_BOTTOM, -4.0);
			slot.icon->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
			slot.icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
			slot.panel->add_child(slot.icon);
		}
		slot.icon->set_texture(slot.item->get_icon());
	}
	if (slot.count_label) {
		slot.count_label->set_text(_show_item_count && slot.item->get_item_amount() > 1
				? String::num_int64(slot.item->get_item_amount()) : String());
	}
	if (slot.panel) slot.panel->set_tooltip_text(slot.item->get_hint_description().is_empty()
				? slot.item->get_name() : slot.item->get_name() + "\n" + slot.item->get_hint_description());
}

void GridInventory::_apply_slot_style(Slot &slot) {
	if (!slot.panel) return;
	const int64_t key = _make_key(slot.column, slot.row);
	Ref<StyleBox> style;
	if (key == _hovered_key && _item_frame_hover.is_valid()) {
		style = _item_frame_hover;
	} else if (Point2i(slot.column, slot.row) == _selected_cell && _item_frame_selected.is_valid()) {
		style = _item_frame_selected;
	} else {
		style = _item_frame;
	}
	if (style.is_valid()) slot.panel->add_theme_stylebox_override("panel", style);
	else slot.panel->remove_theme_stylebox_override("panel");
}

void GridInventory::_generate_grid() {
	_clear_grid();
	if (_slot_size.x < 1 || _slot_size.y < 1 || _rows < 1 || _columns < 1) {
		update_minimum_size();
		queue_redraw();
		return;
	}
	for (int row = 0; row < _rows; ++row) {
		for (int column = 0; column < _columns; ++column) {
			const Point2i position(_grid_padding.x + column * (_slot_size.x + _slot_margin.x),
					_grid_padding.y + row * (_slot_size.y + _slot_margin.y));
			Slot slot;
			slot.rect = Rect2i(position, _slot_size);
			slot.column = column;
			slot.row = row;
			slot.panel = _create_slot_panel(position);
			slot.count_label = _create_count_label();
			slot.panel->add_child(slot.count_label);
			add_child(slot.panel);
			_apply_slot_style(slot);
			_cells.insert(_make_key(column, row), slot);
		}
	}
	_hovered_key = INVALID_KEY;
	update_minimum_size();
	queue_redraw();
}

void GridInventory::_clear_grid() {
	for (KeyValue<int64_t, Slot> &entry : _cells) {
		Slot &slot = entry.value;
		if (slot.panel) {
			Callable entered = callable_mp(this, &GridInventory::_on_slot_mouse_entered);
			Callable exited = callable_mp(this, &GridInventory::_on_slot_mouse_exited);
			Callable gui_input = callable_mp(this, &GridInventory::_on_slot_gui_input);
			if (slot.panel->is_connected("mouse_entered", entered)) slot.panel->disconnect("mouse_entered", entered);
			if (slot.panel->is_connected("mouse_exited", exited)) slot.panel->disconnect("mouse_exited", exited);
			if (slot.panel->is_connected("gui_input", gui_input)) slot.panel->disconnect("gui_input", gui_input);
			slot.panel->queue_free();
		}
	}
	_cells.clear();
}

void GridInventory::_connect_style_signal(const Ref<StyleBox> &style) {
	const Callable changed = callable_mp(this, &GridInventory::_on_style_changed);
	if (style.is_valid() && !style->is_connected("changed", changed)) style->connect("changed", changed);
}

void GridInventory::_disconnect_style_signal(const Ref<StyleBox> &style) {
	const Callable changed = callable_mp(this, &GridInventory::_on_style_changed);
	if (style.is_valid() && style->is_connected("changed", changed)) style->disconnect("changed", changed);
}

Variant GridInventory::_make_drag_data(const Vector2 &) {
	if (!_interaction_enabled) return Variant();
	const int64_t key = _key_at_position(Vector2i(get_local_mouse_position()));
	Slot *slot = _cells.getptr(key);
	if (!slot || slot->item.is_null() || !slot->icon) return Variant();

	Ref<ItemView> transfer_item = _creative_source ? slot->item->duplicate_item() : slot->item;
	Dictionary payload;
	payload["item"] = transfer_item;
	payload["source"] = this;
	payload["source_cell"] = Vector2i(slot->column, slot->row);
	payload["creative_source"] = _creative_source;

	TextureRect *preview_icon = memnew(TextureRect);
	preview_icon->set_texture(transfer_item->get_icon());
	preview_icon->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	preview_icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	preview_icon->set_custom_minimum_size(Vector2(_slot_size));
	set_drag_preview(preview_icon);

	if (!_creative_source) {
		_drag_backup = slot->item;
		_drag_source_slot = slot;
		_clear_slot(*slot);
		emit_signal("item_changed", Vector2i(slot->column, slot->row), Ref<ItemView>());
	}
	return payload;
}

bool GridInventory::_accept_drop_data(const Vector2 &, const Variant &data) const {
	if (!_interaction_enabled || _creative_source || data.get_type() != Variant::DICTIONARY) return false;
	const Dictionary payload = data;
	const Ref<ItemView> item = payload.get("item", Variant());
	if (item.is_null()) return false;
	return _cells.has(_key_at_position(Vector2i(get_local_mouse_position())));
}

void GridInventory::_handle_drop_data(const Vector2 &, const Variant &data) {
	if (_creative_source || data.get_type() != Variant::DICTIONARY) return;
	const Dictionary payload = data;
	const Ref<ItemView> item = payload.get("item", Variant());
	const int64_t target_key = _key_at_position(Vector2i(get_local_mouse_position()));
	Slot *target = _cells.getptr(target_key);
	if (item.is_null() || !target) return;

	const Ref<ItemView> displaced = target->item;
	const bool from_creative = bool(payload.get("creative_source", false));
	if (from_creative) {
		set_item_at(item, Point2i(target->column, target->row));
	} else {
		Object *source_object = payload.get("source", Variant());
		GridInventory *source = Object::cast_to<GridInventory>(source_object);
		const Vector2i source_cell = payload.get("source_cell", Vector2i(-1, -1));
		const Point2i target_cell(target->column, target->row);
		if (displaced.is_valid() && displaced->get_id() == item->get_id()) {
			Ref<ItemView> stacked = displaced;
			stacked->set_item_amount(stacked->get_item_amount() + item->get_item_amount());
			set_item_at(stacked, target_cell);
		} else {
			set_item_at(item, target_cell);
			if (source && displaced.is_valid()) source->set_item_at(displaced, Point2i(source_cell));
		}
		if (source) {
			source->_drag_source_slot = nullptr;
			source->_drag_backup.unref();
		}
	}
}

void GridInventory::_on_slot_gui_input(InputEvent *event) {
	if (!_interaction_enabled) return;
	auto *mouse_event = Object::cast_to<InputEventMouseButton>(event);
	if (!mouse_event || !mouse_event->is_pressed()) return;
	const int64_t key = _key_at_position(Vector2i(get_local_mouse_position()));
	if (const Slot *slot = _cells.getptr(key)) {
		emit_signal("slot_clicked", Vector2i(slot->column, slot->row), mouse_event->get_button_index());
	}
}

void GridInventory::_on_style_changed() { queue_redraw(); }

void GridInventory::_on_label_settings_changed() {
	for (KeyValue<int64_t, Slot> &entry : _cells) {
		if (entry.value.count_label && _count_label_settings.is_valid()) {
			entry.value.count_label->set_label_settings(_count_label_settings);
		}
	}
}

void GridInventory::_on_slot_mouse_entered() {
	_hovered_key = _key_at_position(Vector2i(get_local_mouse_position()));
	if (Slot *slot = _cells.getptr(_hovered_key)) _apply_slot_style(*slot);
}

void GridInventory::_on_slot_mouse_exited() {
	if (Slot *slot = _cells.getptr(_hovered_key)) _apply_slot_style(*slot);
	_hovered_key = INVALID_KEY;
}

Size2 GridInventory::_get_minimum_size() const {
	return Size2(_grid_padding.x * 2 + _columns * _slot_size.x + MAX(0, _columns - 1) * _slot_margin.x,
			_grid_padding.y * 2 + _rows * _slot_size.y + MAX(0, _rows - 1) * _slot_margin.y);
}

Ref<ItemView> GridInventory::get_item_at(const Point2i &cell) const {
	const Slot *slot = _cells.getptr(_make_key(cell.x, cell.y));
	return slot ? slot->item : Ref<ItemView>();
}

bool GridInventory::set_item_at(const Ref<ItemView> &item, const Point2i &cell) {
	Slot *slot = _cells.getptr(_make_key(cell.x, cell.y));
	if (!slot || item.is_null()) return false;
	_clear_slot(*slot);
	slot->item = item;
	_sync_slot(*slot);
	_apply_slot_style(*slot);
	emit_signal("item_changed", cell, item);
	return true;
}

bool GridInventory::clear_item_at(const Point2i &cell) {
	Slot *slot = _cells.getptr(_make_key(cell.x, cell.y));
	if (!slot) return false;
	_clear_slot(*slot);
	_apply_slot_style(*slot);
	emit_signal("item_changed", cell, Ref<ItemView>());
	return true;
}

bool GridInventory::add_item_at(const Ref<ItemView> &item, const Point2i &cell) {
	if (item.is_null()) return false;
	Slot *slot = _cells.getptr(_make_key(cell.x, cell.y));
	if (!slot) return false;
	if (slot->item.is_null()) return set_item_at(item, cell);
	if (slot->item->get_id() != item->get_id()) return false;
	slot->item->set_item_amount(slot->item->get_item_amount() + item->get_item_amount());
	_sync_slot(*slot);
	emit_signal("item_changed", cell, slot->item);
	return true;
}

bool GridInventory::add_item(const Ref<ItemView> &item) {
	if (item.is_null()) return false;
	Slot *first_empty = nullptr;
	for (KeyValue<int64_t, Slot> &entry : _cells) {
		Slot &slot = entry.value;
		if (slot.item.is_valid() && slot.item->get_id() == item->get_id()) {
			return add_item_at(item, Point2i(slot.column, slot.row));
		}
		if (slot.item.is_null() && !first_empty) first_empty = &slot;
	}
	return first_empty && set_item_at(item, Point2i(first_empty->column, first_empty->row));
}

void GridInventory::set_selected_cell(const Point2i &cell) {
	if (cell.x < -1 || cell.x >= _columns || cell.y < -1 || cell.y >= _rows) return;
	const Point2i previous = _selected_cell;
	_selected_cell = cell;
	if (Slot *slot = _cells.getptr(_make_key(previous.x, previous.y))) _apply_slot_style(*slot);
	if (Slot *slot = _cells.getptr(_make_key(cell.x, cell.y))) _apply_slot_style(*slot);
}

void GridInventory::set_creative_source(const bool enabled) { _creative_source = enabled; }

void GridInventory::set_show_item_count(const bool enabled) {
	_show_item_count = enabled;
	for (KeyValue<int64_t, Slot> &entry : _cells) _sync_slot(entry.value);
}

void GridInventory::set_interaction_enabled(const bool enabled) {
	_interaction_enabled = enabled;
	for (KeyValue<int64_t, Slot> &entry : _cells) {
		if (entry.value.panel) entry.value.panel->set_mouse_filter(enabled ? MOUSE_FILTER_STOP : MOUSE_FILTER_IGNORE);
	}
}

void GridInventory::set_rows(const int value) { _rows = MAX(1, value); _generate_grid(); }
void GridInventory::set_columns(const int value) { _columns = MAX(1, value); _generate_grid(); }
void GridInventory::set_slot_size(const Size2i &value) { _slot_size = value; _generate_grid(); }
void GridInventory::set_slot_margin(const Size2i &value) { _slot_margin = value; _generate_grid(); }
void GridInventory::set_grid_padding(const Size2i &value) { _grid_padding = value; _generate_grid(); }

void GridInventory::set_background(const Ref<StyleBox> &value) {
	_disconnect_style_signal(_background);
	_background = value;
	_connect_style_signal(_background);
	queue_redraw();
}
void GridInventory::set_item_frame(const Ref<StyleBox> &value) {
	_disconnect_style_signal(_item_frame);
	_item_frame = value;
	_connect_style_signal(_item_frame);
	for (KeyValue<int64_t, Slot> &entry : _cells) _apply_slot_style(entry.value);
}
void GridInventory::set_item_frame_hover(const Ref<StyleBox> &value) {
	_disconnect_style_signal(_item_frame_hover);
	_item_frame_hover = value;
	_connect_style_signal(_item_frame_hover);
	for (KeyValue<int64_t, Slot> &entry : _cells) _apply_slot_style(entry.value);
}
void GridInventory::set_item_frame_selected(const Ref<StyleBox> &value) {
	_disconnect_style_signal(_item_frame_selected);
	_item_frame_selected = value;
	_connect_style_signal(_item_frame_selected);
	for (KeyValue<int64_t, Slot> &entry : _cells) _apply_slot_style(entry.value);
}
void GridInventory::set_count_label_settings(const Ref<LabelSettings> &value) {
	if (_count_label_settings.is_valid()) {
		Callable changed = callable_mp(this, &GridInventory::_on_label_settings_changed);
		if (_count_label_settings->is_connected("changed", changed)) _count_label_settings->disconnect("changed", changed);
	}
	_count_label_settings = value;
	if (_count_label_settings.is_valid()) _count_label_settings->connect("changed", callable_mp(this, &GridInventory::_on_label_settings_changed));
	for (KeyValue<int64_t, Slot> &entry : _cells) {
		if (entry.value.count_label && _count_label_settings.is_valid()) entry.value.count_label->set_label_settings(_count_label_settings);
	}
}

void GridInventory::_notification(const int what) {
	switch (what) {
		case NOTIFICATION_DRAW: _draw_background(); break;
		case NOTIFICATION_ENTER_TREE:
			if (_cells.is_empty()) _generate_grid();
			_connect_style_signal(_background);
			_connect_style_signal(_item_frame);
			_connect_style_signal(_item_frame_hover);
			_connect_style_signal(_item_frame_selected);
			break;
		case NOTIFICATION_EXIT_TREE:
			_disconnect_style_signal(_background);
			_disconnect_style_signal(_item_frame);
			_disconnect_style_signal(_item_frame_hover);
			_disconnect_style_signal(_item_frame_selected);
			break;
		case NOTIFICATION_DRAG_END:
			if (_drag_source_slot) {
				if (!is_drag_successful()) {
					_drag_source_slot->item = _drag_backup;
					_sync_slot(*_drag_source_slot);
					emit_signal("item_changed", Vector2i(_drag_source_slot->column, _drag_source_slot->row), _drag_backup);
				}
				_drag_source_slot = nullptr;
				_drag_backup.unref();
			}
			break;
	}
}

void GridInventory::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_item_at", "cell"), &GridInventory::get_item_at);
	ClassDB::bind_method(D_METHOD("set_item_at", "item", "cell"), &GridInventory::set_item_at);
	ClassDB::bind_method(D_METHOD("clear_item_at", "cell"), &GridInventory::clear_item_at);
	ClassDB::bind_method(D_METHOD("add_item_at", "item", "cell"), &GridInventory::add_item_at);
	ClassDB::bind_method(D_METHOD("add_item", "item"), &GridInventory::add_item);
	ClassDB::bind_method(D_METHOD("set_selected_cell", "cell"), &GridInventory::set_selected_cell);
	ClassDB::bind_method(D_METHOD("get_selected_cell"), &GridInventory::get_selected_cell);
	ClassDB::bind_method(D_METHOD("set_creative_source", "enabled"), &GridInventory::set_creative_source);
	ClassDB::bind_method(D_METHOD("is_creative_source"), &GridInventory::is_creative_source);
	ClassDB::bind_method(D_METHOD("set_show_item_count", "enabled"), &GridInventory::set_show_item_count);
	ClassDB::bind_method(D_METHOD("is_show_item_count"), &GridInventory::is_show_item_count);
	ClassDB::bind_method(D_METHOD("set_interaction_enabled", "enabled"), &GridInventory::set_interaction_enabled);
	ClassDB::bind_method(D_METHOD("is_interaction_enabled"), &GridInventory::is_interaction_enabled);
	ClassDB::bind_method(D_METHOD("get_rows"), &GridInventory::get_rows);
	ClassDB::bind_method(D_METHOD("set_rows", "value"), &GridInventory::set_rows);
	ClassDB::bind_method(D_METHOD("get_columns"), &GridInventory::get_columns);
	ClassDB::bind_method(D_METHOD("set_columns", "value"), &GridInventory::set_columns);
	ClassDB::bind_method(D_METHOD("get_slot_size"), &GridInventory::get_slot_size);
	ClassDB::bind_method(D_METHOD("set_slot_size", "value"), &GridInventory::set_slot_size);
	ClassDB::bind_method(D_METHOD("get_slot_margin"), &GridInventory::get_slot_margin);
	ClassDB::bind_method(D_METHOD("set_slot_margin", "value"), &GridInventory::set_slot_margin);
	ClassDB::bind_method(D_METHOD("get_grid_padding"), &GridInventory::get_grid_padding);
	ClassDB::bind_method(D_METHOD("set_grid_padding", "value"), &GridInventory::set_grid_padding);
	ClassDB::bind_method(D_METHOD("get_background"), &GridInventory::get_background);
	ClassDB::bind_method(D_METHOD("set_background", "value"), &GridInventory::set_background);
	ClassDB::bind_method(D_METHOD("get_item_frame"), &GridInventory::get_item_frame);
	ClassDB::bind_method(D_METHOD("set_item_frame", "value"), &GridInventory::set_item_frame);
	ClassDB::bind_method(D_METHOD("get_item_frame_hover"), &GridInventory::get_item_frame_hover);
	ClassDB::bind_method(D_METHOD("set_item_frame_hover", "value"), &GridInventory::set_item_frame_hover);
	ClassDB::bind_method(D_METHOD("get_item_frame_selected"), &GridInventory::get_item_frame_selected);
	ClassDB::bind_method(D_METHOD("set_item_frame_selected", "value"), &GridInventory::set_item_frame_selected);
	ClassDB::bind_method(D_METHOD("get_count_label_settings"), &GridInventory::get_count_label_settings);
	ClassDB::bind_method(D_METHOD("set_count_label_settings", "value"), &GridInventory::set_count_label_settings);

	ADD_SIGNAL(MethodInfo("slot_clicked", PropertyInfo(Variant::VECTOR2I, "cell"), PropertyInfo(Variant::INT, "button_index")));
	ADD_SIGNAL(MethodInfo("item_changed", PropertyInfo(Variant::VECTOR2I, "cell"), PropertyInfo(Variant::OBJECT, "item", PROPERTY_HINT_RESOURCE_TYPE, "ItemView")));
	ADD_PROPERTY(PropertyInfo(Variant::INT, "rows", PROPERTY_HINT_RANGE, "1,20,1"), "set_rows", "get_rows");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "columns", PROPERTY_HINT_RANGE, "1,20,1"), "set_columns", "get_columns");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "slot_size"), "set_slot_size", "get_slot_size");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "slot_margin"), "set_slot_margin", "get_slot_margin");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "grid_padding"), "set_grid_padding", "get_grid_padding");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "creative_source"), "set_creative_source", "is_creative_source");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_item_count"), "set_show_item_count", "is_show_item_count");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "interaction_enabled"), "set_interaction_enabled", "is_interaction_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "background", PROPERTY_HINT_RESOURCE_TYPE, "StyleBox"), "set_background", "get_background");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "item_frame", PROPERTY_HINT_RESOURCE_TYPE, "StyleBox"), "set_item_frame", "get_item_frame");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "item_frame_hover", PROPERTY_HINT_RESOURCE_TYPE, "StyleBox"), "set_item_frame_hover", "get_item_frame_hover");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "item_frame_selected", PROPERTY_HINT_RESOURCE_TYPE, "StyleBox"), "set_item_frame_selected", "get_item_frame_selected");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "count_label_settings", PROPERTY_HINT_RESOURCE_TYPE, "LabelSettings"), "set_count_label_settings", "get_count_label_settings");
}

} // namespace godot
