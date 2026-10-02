# Inventários: núcleo lógico e apresentação

## Responsabilidades

- `src/inventory_service.*`: singleton C++ genérico com registro por ID, capacidade, slots `{id, amount}`, limite por tipo e revisão. Não conhece nós, grids, ItemView, blocos, jogador, mundo, mouse, receitas ou arquivos. Usa os tipos de valor e sinais da Godot como interface com GDScript; não guarda ponteiros de nodes.
- `inventory_session.gd`: autoload que conhece o jogo. Registra os tipos de item, cria UUIDs, define hotbar/storage/catálogo criativo, produz ItemViews e salva/carrega registros por mundo através de SaveService.
- `inventory_drag_controller.gd`: associa grids a UUIDs usando referências fracas, acompanha o arraste e chama o núcleo. Observa notificações e reconstrói as representações.
- `GridInventory`: apresenta snapshots de ItemView e informa eventos da GUI. Colunas, ícone, contador, tooltip e preview são visuais. Não consulta o serviço nem executa regras de inventário.
- `inventory_manager.gd`: autoload de controle do mouse/F1 e seleção da hotbar. A seleção consulta dados lógicos, independentemente da apresentação.

## Cadastro e vínculo

```gdscript
var uuid := InventorySession.create_uuid()
InventorySession.register_inventory(uuid, 27)
drag_controller.bind_grid(grid, uuid)
```

O cadastro no núcleo é puramente lógico. O adaptador GDScript associa registros persistentes ao mundo atual. Duas grids podem representar o mesmo UUID com layouts diferentes. Fechar ou destruir uma grid não remove o cadastro nem os itens.

Para registros sem persistência do jogo, use `InventoryService.register_inventory(id, capacity, copy_source)` diretamente. O núcleo aceita um ID estável fornecido pelo chamador e tipos registrados com `register_item_type(item_id, stack_limit)`. Não impõe a geração de UUID nem um limite universal de 99; essa configuração pertence ao jogo.

## Fluxo de arraste

1. A grid emite `drag_started(index)` com índice linear estável.
2. O controlador consulta o UUID associado e guarda origem, quantidade e revisão. Entrega à GUI apenas um token transitório. A representação da origem fica escondida; seus dados continuam no núcleo e nos snapshots de salvamento.
3. O encaminhamento nativo de drag-and-drop da Godot identifica a grid/célula sob o mouse. A grid emite `drop_hovered(payload, index)` e `drop_requested(payload, index)`.
4. O controlador verifica token, visibilidade e interação das views. Solicita `transfer(source, from, target, to, amount, revision)` ao núcleo.
5. O núcleo revalida índices, quantidade, revisão e compatibilidade. Confirma todas as alterações antes de emitir `inventory_changed(uuid, index)`. A prévia `can_transfer` nunca substitui essa validação final.
6. O controlador lê os dados confirmados e atualiza todas as grids vinculadas. Índice -1 indica atualização de vários slots.

Empilhamento pode mover somente o que cabe, mantendo a sobra na origem. Troca de tipos diferentes exige a pilha inteira. Fontes configuradas como cópia alimentam inventários sem consumir seu conteúdo e rejeitam entradas. `add_items` é uma inserção integral: capacidade insuficiente não altera nenhum slot.

Drop inválido, destino cheio, F1, fechamento da origem ou fim do gesto sem destino cancelam o estado transitório. A atualização restaura ID, quantidade, nome, categoria, ícone e tooltip a partir dos dados atuais. Uma alteração da origem durante o arraste invalida a revisão anterior. O controlador consome o token e impede reaproveitamento após conclusão/cancelamento.

O núcleo executa essas operações sincronamente na thread principal. Atomicidade aqui significa que a validação antecede a mutação e que observadores recebem sinais somente depois de ambos os inventários terem sido atualizados; a API não oferece acesso concorrente por workers.

## Colunas e capacidade

As janelas calculam colunas pela largura útil, com mínimo de duas e tamanho de slot preservado. A atualização é agrupada por frame. Alterar colunas não altera capacidade, ordem ou identidade: o slot 8 passa de (8, 0) para (0, 4) com duas colunas. Uma capacidade de 27 não cria slots extras na última linha. A hotbar mantém nove colunas.

## Persistência e remoção do crafting

VoxelAPI emite `world_opened(id)` e `world_saving(id)`. InventorySession conecta esses sinais e carrega/salva mesmo sem jogador ou UI. A prévia do menu não abre nem salva inventários persistentes.

A seção `inventories` do mundo guarda versão 3, IDs por papel, seleção e registros por UUID com capacidade e slots. Os IDs são mantidos nas cargas seguintes. O catálogo criativo é reconstruído sem persistir como inventário consumível. Registros permanecem em memória durante a sessão.

A interface e as regras de crafting foram removidas. Saves antigos com ingredientes nos slots de craft são migrados para um registro lógico de recuperação. Os itens voltam ao storage/hotbar assim que existe espaço; enquanto tudo está cheio, permanecem salvos nesse registro. Um UUID antigo de craft é preservado como UUID de recuperação. Mundos novos não criam inventário de craft.

## Verificação

- `inventory_service_test.gd`: núcleo sem UI, limites configuráveis, snapshots, transferências parciais, troca, revisão obsoleta, falhas sem mutação e dados completos na primeira notificação.
- `inventory_controller_test.gd`: rejeição de drop e ressincronização de ItemView, destinos ocultos, invalidação por mutação, tokens consumidos, múltiplas views e ciclo de vida independente.
- `inventory_ui_test.gd` e `inventory_mouse_test.gd`: encaminhamento de eventos reais da GUI, F1, mouse, ícones, contadores, scroll e janelas.
- `inventory_reflow_test.gd`: mudança de colunas durante arraste, capacidade fixa, ordem e permanência do modelo após destruir a UI.
- `inventory_session_test.gd`: migração de crafting com inventário cheio, recuperação posterior, UUIDs fixos e isolamento entre mundos.
- `inventory_disk_test.gd`: escrita e leitura em processos separados, com inventário adicional sem UI.
