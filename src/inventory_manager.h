#ifndef INVENTORY_MANAGER_H
#define INVENTORY_MANAGER_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/node.hpp>

namespace godot {

class GridInventory;
class ItemView;
class PanelContainer;

class InventoryManager final : public Object {
	GDCLASS(InventoryManager, Object)

	static InventoryManager *singleton;
	Node *_player = nullptr;
	Control *_ui = nullptr;
	PanelContainer *_creative_panel = nullptr;
	GridInventory *_creative_grid = nullptr;
	GridInventory *_inventory_grid = nullptr;
	GridInventory *_hotbar_grid = nullptr;
	int _selected_slot = 0;
	int _selected_block_id = 0;
	bool _inventory_open = false;

	void _on_ui_tree_exiting();
	void _on_hotbar_slot_clicked(const Vector2i &cell, int button_index);
	void _on_hotbar_item_changed(const Vector2i &cell, const Ref<ItemView> &item);
	void _update_selected_item();
	void _disconnect_grids();

protected:
	static void _bind_methods();

public:
	InventoryManager();
	~InventoryManager();
	static InventoryManager *get_singleton() { return singleton; }

	void setup(Node *player, Control *ui);
	void toggle_inventory();
	void open_inventory();
	void close_inventory();
	bool is_inventory_open() const { return _inventory_open; }
	void select_hotbar_slot(int slot);
	int get_selected_hotbar_slot() const { return _selected_slot; }
	int get_selected_block_id() const { return _selected_block_id; }
};

} // namespace godot

#endif // INVENTORY_MANAGER_H
