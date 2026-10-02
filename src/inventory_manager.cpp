#include "inventory_manager.h"

#include "grid_inventory.h"
#include "item_view.h"

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

InventoryManager *InventoryManager::singleton = nullptr;

InventoryManager::InventoryManager() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
}

InventoryManager::~InventoryManager() {
	if (singleton == this) singleton = nullptr;
}

void InventoryManager::_disconnect_grids() {
	if (_hotbar_grid) {
		const Callable clicked = callable_mp(this, &InventoryManager::_on_hotbar_slot_clicked);
		const Callable changed = callable_mp(this, &InventoryManager::_on_hotbar_item_changed);
		if (_hotbar_grid->is_connected("slot_clicked", clicked)) _hotbar_grid->disconnect("slot_clicked", clicked);
		if (_hotbar_grid->is_connected("item_changed", changed)) _hotbar_grid->disconnect("item_changed", changed);
	}
}

void InventoryManager::setup(Node *player, Control *ui) {
	if (_ui && is_inventory_open()) close_inventory();
	_disconnect_grids();
	if (_ui) {
		const Callable exiting = callable_mp(this, &InventoryManager::_on_ui_tree_exiting);
		if (_ui->is_connected("tree_exiting", exiting)) _ui->disconnect("tree_exiting", exiting);
	}

	_player = player;
	_ui = ui;
	_creative_panel = nullptr;
	_creative_grid = nullptr;
	_inventory_grid = nullptr;
	_hotbar_grid = nullptr;
	_mouse_unlocked = false;
	_selected_slot = 0;
	_selected_block_id = 0;

	if (!_player || !_ui) {
		UtilityFunctions::push_error("InventoryManager.setup needs a player and an inventory UI Control");
		return;
	}

	_creative_panel = Object::cast_to<PanelContainer>(_ui->get_node_or_null("CreativePanel"));
	_creative_grid = Object::cast_to<GridInventory>(_ui->get_node_or_null("CreativePanel/Margin/Content/CreativeScroll/Center/CreativeGrid"));
	_inventory_grid = Object::cast_to<GridInventory>(_ui->get_node_or_null("InventoryPanel/Margin/Content/InventoryScroll/Center/InventoryGrid"));
	_hotbar_grid = Object::cast_to<GridInventory>(_ui->get_node_or_null("HotbarPanel/Margin/HotbarGrid"));
	if (!_creative_panel || !_creative_grid || !_inventory_grid || !_hotbar_grid) {
		UtilityFunctions::push_error("InventoryManager could not find CreativePanel, CreativeGrid, and HotbarGrid in the inventory scene");
		_player = nullptr;
		_ui = nullptr;
		return;
	}

	_ui->connect("tree_exiting", callable_mp(this, &InventoryManager::_on_ui_tree_exiting));
	_hotbar_grid->connect("slot_clicked", callable_mp(this, &InventoryManager::_on_hotbar_slot_clicked));
	_hotbar_grid->connect("item_changed", callable_mp(this, &InventoryManager::_on_hotbar_item_changed));
	_creative_panel->hide();
	_creative_grid->set_interaction_enabled(false);
	_inventory_grid->set_interaction_enabled(false);
	_hotbar_grid->set_interaction_enabled(false);
	_ui->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	_hotbar_grid->set_selected_cell(Point2i(_selected_slot, 0));
	_update_selected_item();
	set_mouse_unlocked(false);
}

void InventoryManager::_on_ui_tree_exiting() {
	_disconnect_grids();
	_player = nullptr;
	_ui = nullptr;
	_creative_panel = nullptr;
	_creative_grid = nullptr;
	_inventory_grid = nullptr;
	_hotbar_grid = nullptr;
	_mouse_unlocked = false;
	_selected_block_id = 0;
}

void InventoryManager::_update_selected_item() {
	if (!_hotbar_grid) {
		_selected_block_id = 0;
		return;
	}
	const Ref<ItemView> item = _hotbar_grid->get_item_at(Point2i(_selected_slot, 0));
	_selected_block_id = item.is_valid() ? item->get_id() : 0;
}

void InventoryManager::_on_hotbar_slot_clicked(const Vector2i &cell, const int button_index) {
	if (!_mouse_unlocked || button_index != MOUSE_BUTTON_LEFT) return;
	select_hotbar_slot(cell.x);
}

void InventoryManager::_on_hotbar_item_changed(const Vector2i &cell, const Ref<ItemView> &) {
	if (cell.y == 0 && cell.x == _selected_slot) _update_selected_item();
}

bool InventoryManager::is_inventory_open() const {
    if (!_ui) return false;
    Control *inventory = Object::cast_to<Control>(_ui->get_node_or_null("InventoryPanel"));
    return (_creative_panel && _creative_panel->is_visible()) || (inventory && inventory->is_visible());
}

void InventoryManager::toggle_mouse() { set_mouse_unlocked(!_mouse_unlocked); }

void InventoryManager::set_mouse_unlocked(bool unlocked) {
    if (!_ui) return;
    _mouse_unlocked = unlocked;
    if (_ui->has_method("set_mouse_unlocked")) _ui->call("set_mouse_unlocked", unlocked);
    Input::get_singleton()->set_mouse_mode(unlocked ? Input::MOUSE_MODE_VISIBLE : Input::MOUSE_MODE_CAPTURED);
}

void InventoryManager::toggle_inventory() {
    Control *inventory = _ui ? Object::cast_to<Control>(_ui->get_node_or_null("InventoryPanel")) : nullptr;
    if (inventory && inventory->is_visible()) inventory->hide();
    else open_inventory();
}

void InventoryManager::open_inventory() {
    if (!_ui) return;
    set_mouse_unlocked(true);
    if (Control *inventory = Object::cast_to<Control>(_ui->get_node_or_null("InventoryPanel"))) inventory->show();
}

void InventoryManager::close_inventory() {
    if (!_ui) return;
    if (_creative_panel) _creative_panel->hide();
    if (Control *inventory = Object::cast_to<Control>(_ui->get_node_or_null("InventoryPanel"))) inventory->hide();
}

void InventoryManager::select_hotbar_slot(const int slot) {
	if (!_hotbar_grid) return;
	const int columns = _hotbar_grid->get_columns();
	if (columns < 1) return;
	_selected_slot = ((slot % columns) + columns) % columns;
	_hotbar_grid->set_selected_cell(Point2i(_selected_slot, 0));
	_update_selected_item();
}

void InventoryManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup", "player", "ui"), &InventoryManager::setup);
	ClassDB::bind_method(D_METHOD("toggle_inventory"), &InventoryManager::toggle_inventory);
	ClassDB::bind_method(D_METHOD("open_inventory"), &InventoryManager::open_inventory);
	ClassDB::bind_method(D_METHOD("close_inventory"), &InventoryManager::close_inventory);
	ClassDB::bind_method(D_METHOD("is_inventory_open"), &InventoryManager::is_inventory_open);
	ClassDB::bind_method(D_METHOD("is_mouse_unlocked"), &InventoryManager::is_mouse_unlocked);
	ClassDB::bind_method(D_METHOD("set_mouse_unlocked", "unlocked"), &InventoryManager::set_mouse_unlocked);
	ClassDB::bind_method(D_METHOD("toggle_mouse"), &InventoryManager::toggle_mouse);
	ClassDB::bind_method(D_METHOD("select_hotbar_slot", "slot"), &InventoryManager::select_hotbar_slot);
	ClassDB::bind_method(D_METHOD("get_selected_hotbar_slot"), &InventoryManager::get_selected_hotbar_slot);
	ClassDB::bind_method(D_METHOD("get_selected_block_id"), &InventoryManager::get_selected_block_id);
}

} // namespace godot
