# Dia, noite e detalhes do mundo

## Ciclo de iluminação

`project/scenes/directional_light_3d.gd` controla um ciclo de 20 minutos:
13 minutos entre 06:00 e 18:00 e 7 minutos entre 18:00 e 06:00. Começa às
08:00, respeitando o valor `hora` configurado. `duracao_dia` representa o ciclo
completo e `proporcao_dia` controla a fração diurna; ambos são editáveis no
Inspector. `set_hour()` permite pré-visualizar um horário imediatamente.

O sol fica acima do terreno ao meio-dia e deixa de iluminar à noite. A luz
ambiente passa de 0,45 durante o dia a 0,075 à noite, com tonalidade fria.
Uma luz lunar fraca (energia 0,045) mantém um mínimo de orientação sem uma
segunda renderização de sombras. Céu e neblina acompanham a iluminação, com
transições quentes no horizonte. A água acompanha `world_daylight` para não
ficar clara por usar um shader unshaded. O ambiente é duplicado para evitar
alterações em recursos compartilhados. Parâmetros visuais mudam a 10 Hz.

## Texturas e vegetação

Os minérios de ferro e diamante têm texturas próprias de rocha com inclusões,
em vez de reutilizar pedra com tint uniforme. Troncos de carvalho usam
`oak_log_side` na casca e `oak_log_top` no topo e na base, com anéis de crescimento.
As novas plantas são margarida, centáurea, papoula, grama baixa e samambaia.
As texturas continuam em RGBA 32 × 32, no atlas gerado pelo Block Registry.
IDs existentes são preservados; as novas plantas usam IDs 17–21.

Fontes e prompts dos assets gerados ficam registrados em
`project/art/texture_prompts.json`. Os PNGs usados pelo jogo estão em
`project/textures/blocks/`. O cache de ícones invalida as imagens automaticamente
quando as texturas importadas ou o registro mudam.

Regenerar registro, IDs C++ e atlas depois de alterações:

```sh
XDG_DATA_HOME=/tmp/godot-polish-tests godot --headless --path project --script res://scripts/tools/rebuild_block_assets.gd
scons -j4
```

Testar dia/noite:

```sh
XDG_DATA_HOME=/tmp/godot-polish-tests godot --headless --path project --log-file /tmp/godot-day-night.log --script res://tests/day_night_test.gd
```

Para comparar visualmente os novos minérios e troncos, use o inventário.
Para avaliar a distribuição das novas plantas sem edições antigas, crie um
mundo novo. Escuridão e cores ainda dependem do monitor e do renderizador.

Teste determinístico da distribuição de plantas:

```sh
c++ -std=c++17 -O2 -Wall -Wextra -pedantic tests/surface_vegetation_test.cpp -o /tmp/surface_vegetation_test
/tmp/surface_vegetation_test
```

Validação realizada: importação pelo editor, geração de atlas e IDs, build
SCons, ciclo de iluminação, distribuição de plantas em três seeds,
dimensões/transparência das texturas e integração dos ícones com o inventário.
Os testes de iluminação usam o Godot headless; não medem aparência final
em uma sessão com renderização gráfica.
