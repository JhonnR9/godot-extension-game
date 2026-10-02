# Inventário e controle do mouse

F1 (`unlock_mouse`) alterna entre controle do personagem e mouse livre. Com o mouse livre, movimento, câmera e interação com blocos ficam suspensos; F1 novamente devolve o controle sem fechar as janelas. O atalho de salvar o mundo passa a F5 para evitar conflito.

O baú fechado no canto superior esquerdo abre ou fecha o inventário do jogador. O botão criativo ao lado controla uma janela separada. As duas podem permanecer abertas simultaneamente, com crafting disponível no inventário do jogador. A hotbar mantém seu funcionamento e aparência.

Arraste uma janela pela barra de título; arraste o canto inferior direito para redimensionar. Posição e tamanho são salvos nas configurações locais `inventory_windows`. Os painéis são limitados à área da tela, mantêm rolagem para itens que não cabem e preservam os itens ao redimensionar a viewport.

`InventoryManager` mantém o estado de controle do mouse separado da visibilidade das janelas. `is_inventory_open()` consulta a visibilidade; `is_mouse_unlocked()` decide se o jogador pode receber comandos. A UI ignora eventos de mouse enquanto ele está capturado, incluindo controles internos de rolagem; cancelar a interação interrompe arrastes sem consumir itens.

`floating_inventory_panel.gd` concentra arraste, redimensionamento e limites de tela. `inventory_ui.gd` mantém conteúdo, crafting, janelas independentes e persistência do layout. O F1 é processado em `_input` do jogador, antes da GUI, e não repete ao manter a tecla pressionada.

Validação: `inventory_mouse_test.gd` verifica F1, clique nos lançadores, mover e redimensionar, persistência, passagem de eventos e controle do personagem com as janelas abertas. `inventory_ui_test.gd` cobre drag-and-drop, empilhamento, inventário cheio, salvamento e redimensionamento da viewport.
