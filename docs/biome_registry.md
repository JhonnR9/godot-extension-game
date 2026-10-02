# Registro de biomas e camadas

O arquivo `project/data/biome_registry.json` configura o relevo, os materiais,
as camadas, as árvores e a vegetação. As definições existentes continuam sendo
planície, deserto, oceano, rio e praia, com os parâmetros anteriores.

`VoxelAPI` lê e valida o arquivo uma vez em `_ready()`. Cada tarefa de geração
recebe uma referência compartilhada e imutável ao registro. Não há leitura de
JSON, de disco ou recarregamento de configurações durante a geração dos chunks.
Reinicie o mundo/jogo após editar o arquivo. Para usar outro arquivo, configure
`biome_registry_path` no Inspector ou antes de adicionar `VoxelAPI` à árvore.

Uma configuração inválida gera uma mensagem com o caminho e o campo/problema,
e o jogo usa os cinco biomas padrão embutidos. É possível validar sem iniciar
um mundo usando `VoxelAPI.validate_biome_registry(dictionary)`, que retorna
`valid` e `error`. IDs e nomes precisam ser únicos; materiais precisam existir
no registro de blocos; intervalos, frequências e limites são validados.

## Seleção e transições

O ruído `climate` é convertido em um peso entre 0 e 1. Os biomas de tipo `land`
particionam esse intervalo usando `selection.climate_min`/`climate_max`, sem
lacunas ou sobreposições. O limite superior é exclusivo, exceto no valor 1.
`relief.anchor` posiciona cada perfil de relevo nesse mesmo eixo. A altura é
interpolada entre as duas âncoras vizinhas, evitando degraus entre biomas.
As âncoras precisam ser únicas.

`selection.kind` também aceita `ocean`, `river` e `beach`. Esses perfis são
selecionados sobre o bioma terrestre conforme a altura relativa ao nível do
mar e a influência do ruído correspondente. `height_max_offset` e
`min_influence` configuram os limites. `climate_min`/`climate_max` podem limitar
cada perfil costeiro a certos climas. Quando mais de um perfil do mesmo tipo
combina, vence a maior `priority`; empates usam a ordem no arquivo.

A seleção dos materiais usa manchas suaves de aproximadamente oito blocos,
determinísticas pela semente e coordenadas mundiais. Uma faixa de até 0,10 de
clima de cada lado dos limites mistura terra/grama, areia e rocha sem recortar
o relevo. Isso também suaviza os limites de `surface_overrides`.

A ordem de seleção é oceano, rio e praia. `dry_coast` mantém uma plataforma
arenosa rasa no início da costa e desativa rios no deserto. A influência
costeira é interpolada entre climas; a plataforma desce gradualmente até o
mesmo fundo oceânico das costas úmidas. Oceano e praia podem substituir o
deserto, permitindo água no litoral. `dry_coast_clamp` e `coast_blend_start/end`
são aceitos por compatibilidade, mas não controlam mais essa transição.
`surface_water` permite controlar o preenchimento de
água independentemente do ID/nome do bioma.

As mudanças afetam chunks novos; chunks já salvos conservam o terreno anterior.

O bloco `world` configura altura base, nível do mar, amplitude, transições de
costa, profundidade do leito dos rios e os seis ruídos. As sementes e seus
deslocamentos continuam determinísticos por mundo. A altura final fica limitada
a Y = -255 até 255, respeitando a bedrock e o teto do mundo.

## Raridade regional

Cada entrada de bioma possui `rarity`, um inteiro de 0 a 10000:

- `0`: desativa o bioma, inclusive seu perfil de relevo.
- `1`: sempre disponível quando as condições climáticas/costeiras combinam.
- `2`, `4`, `8` etc.: valores maiores tornam a ocorrência progressivamente mais rara.

Por exemplo, `"rarity": 4` no deserto mantém o requisito de clima seco e também
exige uma região aprovada pela raridade. Esse valor não representa uma porcentagem
exata da área do mapa. A seleção utiliza um campo regional suave, com células de
256 blocos, calculado pela semente, pelo ID do bioma e pelas coordenadas mundiais.
Não há sorteio por bloco/chunk nem dependência da ordem de carregamento.

Se um bioma terrestre for recusado, o motor usa o bioma terrestre de `rarity: 1`
com a âncora de relevo mais próxima do clima atual. Por isso, ao menos um bioma
terrestre precisa manter `rarity: 1`; a planície é o fallback padrão. O relevo é
misturado suavemente com esse fallback na transição regional. Para oceano, rio e
praia, a seleção pula os perfis recusados e considera o próximo perfil compatível.

Campo ausente equivale a `1`. As entradas atuais começam com `1` para preservar
a geração existente; altere o valor dos biomas que deseja tornar raros. Reinicie
o jogo após editar. Alterar o ID ou a semente muda o padrão das regiões.

## Materiais e camadas

Os materiais são nomes do registro de blocos, por exemplo `grass`, `dirt`,
`stone`, `sandstone` e `deepslate`. Para cada coluna, a precedência é:

1. Bedrock na base do mundo, Y = -256.
2. `materials.surface` na altura da superfície.
3. `materials.soil` nas próximas `soil_depth` camadas.
4. Primeira entrada de `strata` cujo intervalo de profundidade e Y combine.
5. `materials.deep_rock` abaixo de `deep_rock_below_y`.
6. `materials.rock` no restante.

`strata` aceita `block`, `min_depth`, `max_depth`, `min_y` e `max_y`. Profundidade
é a distância em blocos abaixo da superfície daquela coluna; Y é a altura
absoluta. Os limites são inclusivos. Camadas adicionais não substituem a
superfície, o solo ou a bedrock. Exemplo de faixa de arenito sob o solo:

```json
"strata": [
  {"block": "sandstone", "min_depth": 16, "max_depth": 24}
]
```

`surface_overrides` altera a superfície e opcionalmente `soil_depth` em uma
subfaixa climática. A primeira regra correspondente vence. A faixa de terra
entre planície e deserto usa esse mecanismo.

Cavernas e depósitos continuam em passes separados, depois do preenchimento
das camadas. Depósitos substituem apenas pedra/deepslate; uma camada de arenito
configurada não recebe esses depósitos. As regras de túneis e depósitos seguem
em `src/cave_tunnels.h` e `src/underground_deposits.h`.

## Árvores e plantas

`trees` configura `max_per_chunk`, `min_height`, `max_height`, `crown_radius`,
`trunk`, `leaves` e uma faixa climática opcional. O valor máximo limita candidatos
por coluna de chunk, não garante uma quantidade exata: água e biomas inaptos
podem descartar candidatos. Árvores mantêm a copa em cinco camadas do modelo
atual de carvalho com `shape: "oak"` (padrão). `shape: "palm"` usa um tronco
alto com oito folhas radiais que descem nas pontas. A praia configura palmeiras
com `palm_log`/`palm_leaves`, altura de 7 a 10 e até dois candidatos por chunk.
Só colunas secas acima da água recebem árvores; deserto e oceano não possuem
candidatos de palmeira. Os blocos da árvore original são `oak_log`, `oak_leaves`
e `oak_wood`; seus IDs 8, 9 e 4 foram preservados para mundos salvos.

`vegetation` configura `patch_size`, `coverage_min`/`coverage_max` e
`flowers_min`/`flowers_max`. Cobertura usa milésimos: 120 significa 12% das
colunas elegíveis. Flores fazem parte da cobertura total, não são somadas a ela.
`plants` e `flowers` são listas de `{block, weight, min_height, max_height}`;
pesos controlam a frequência relativa. Plantas só aparecem acima de superfícies
secas. Alturas variáveis permitem cactos ou outras plantas empilhadas.

## Adicionar um bioma

Duplique uma entrada terrestre, atribua ID/nome novos e ajuste a partição
climática das entradas vizinhas. Por exemplo: planície `[0, 0.35)`, floresta
`[0.35, 0.68)` e deserto `[0.68, 1]`, com âncoras 0, 0.5 e 1. Depois configure
materiais, solo, relevo, árvores e plantas. Não é necessário adicionar branches
por ID no gerador. O sistema continua usando um eixo climático; temperatura,
umidade ou biomas verticais independentes exigem ampliar a seleção.

Em exports, inclua `data/*.json` no filtro de arquivos não reconhecidos como
recursos, além dos recursos/scenes usados pelo jogo. Arquivos alternativos fora
dessa pasta também precisam entrar no filtro de exportação.

## Consulta e verificação

`VoxelAPI.sample_terrain_column(Vector2i(x, z))` retorna a altura, o ID/nome do
bioma, o peso climático, materiais, profundidade do solo, nível da água e
permissão de árvores. Use após `_ready()`; iniciar o mundo aplica sua semente.
Essa consulta descreve o terreno base, antes de túneis, depósitos, árvores e
alterações salvas. `TerrainSampler` é compartilhado pela geração e por consultas
dentro/fora do chunk. Árvores não usam mais uma fórmula de altura diferente nas
bordas. O spawn inicial também consulta o terreno real, já com a semente do mundo.
Chunks no teto/base não aguardam vizinhos além dos limites verticais.

```sh
scons -j4
XDG_DATA_HOME=/tmp/godot-biome-check godot --headless --path project --script res://tests/biome_registry_test.gd --log-file /tmp/biome-check.log
c++ -std=c++17 -O2 tests/surface_vegetation_test.cpp -o /tmp/surface_vegetation_test
/tmp/surface_vegetation_test
c++ -std=c++17 -O2 tests/biome_rarity_test.cpp -o /tmp/biome_rarity_test
/tmp/biome_rarity_test
# Execute o teste de raridade em processos separados com argumentos 0, 1, 2 e 4.
XDG_DATA_HOME=/tmp/godot-rarity-check godot --headless --path project --script res://tests/biome_rarity_world_test.gd --log-file /tmp/rarity-check.log -- 4
```

Os testes verificam configurações inválidas, um bioma novo com camadas
personalizadas, alturas e blocos em coordenadas negativas/bordas, os cinco
biomas existentes, árvores com copa/material configurados em ordens diferentes
de geração e spawn acima de um relevo personalizado alto.
