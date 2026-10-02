# Seleção e coleta de objetos voxel

Objetos com `BLOCK_FLAG_CROSSED` (ou a flag padrão do registro) recebem seletores automaticamente durante a construção do mesh. Isso inclui tochas, flores e gramas. Novos objetos cruzados seguem o mesmo caminho.

`ChunkMeshBuilder` produz posições locais dos objetos; o resultado assíncrono leva esses dados ao `ChunkNode`. `set_selection_positions` gerencia um `StaticBody3D` chamado `ObjectSelection` por chunk, com um shape owner por objeto e uma única caixa compartilhada. Não existem nós de colisão por planta, nem dependência dos nós de luz.

A camada 1 representa colisão do terreno; a camada 2 representa seleção de objetos, com máscara zero. O jogador consulta ambas para interação, mantendo o movimento na camada 1. As caixas ocupam o voxel inteiro para facilitar a mira, inclusive nas partes transparentes das texturas.

O índice `shape` do raycast corresponde à lista `voxel_selection_positions` no corpo. O jogador transforma a origem local em posição mundial para quebrar exatamente o voxel atingido, inclusive em chunks distantes ou negativos. Ao colocar outro bloco, soma a normal à origem desse voxel.

A coleta de objetos selecionáveis adiciona uma unidade à hotbar ou ao inventário antes de removê-los. Se ambos estiverem cheios, preserva o objeto no mundo. Blocos sólidos mantêm o comportamento de quebra existente.

Posições idênticas preservam os seletores durante atualizações do mesh. Remoção, descarregamento e reutilização do chunk limpam os shapes; a mesma limpeza também ocorre para chunks que ficam vazios.

Validação: `torch_test.gd` verifica seleção das sete plantas, mapeamento do voxel, separação das camadas, remoção após quebra, coleta, tochas e persistência. Testes de inventário e pipeline também passam em modo headless.
