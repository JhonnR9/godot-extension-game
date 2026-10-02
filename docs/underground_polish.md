# Pedra, emissão e cavernas

## Texturas

A pedra agora usa `res://textures/blocks/stone.png` nas três faces, com grão
neutro compatível com a rocha das texturas dos minérios. Pedra profunda e
bedrock reutilizam essa imagem com seus tints existentes. Carvão usa a nova
`res://textures/blocks/coal_ore.png`, com pequenas inclusões de grafite escuro.
Os PNGs são RGBA 32 × 32. O bloco de carvão usa ID 22; IDs anteriores ficam
preservados. Registro, atlas, header C++ e cache de ícones são atualizados.

As duas imagens foram geradas pela ferramenta integrada `image_gen`, usando
ferro e diamante como referências de estilo, e reduzidas com nearest-neighbor.
Prompts completos e caminhos finais estão em
`project/art/underground_texture_prompts.json`.

## Emissão

`chunk.gdshader` identifica apenas as camadas de ferro e diamante. Duas
máscaras analíticas em RGB linear selecionam, respectivamente, pixels quentes
cobre/tan e pixels frios ciano. A rocha cinza ao redor não emite. A técnica não
precisa de texturas de máscara, amostras extras do atlas ou luzes por bloco.
As intensidades iniciais são 0,65 para ferro e 1,25 para diamante, editáveis
nos uniforms `iron_emission` e `diamond_emission` do shader.

O cenário habilita um glow discreto (intensidade 0,18, limiar HDR 1,1) para os
pontos mais claros. A emissão deixa o minério visível no escuro; ela não é uma
luz pontual que ilumina as paredes vizinhas. O halo depende do suporte a glow
do renderizador. A iluminação e o ciclo noturno continuam ativos.

Os índices de camadas vêm da função C++ gerada `texture_layer_from_name()`;
adicionar/reordenar texturas não troca o minério que emite. `ChunkNode` também
preserva seus materiais ao receber colisões e ao ser reutilizado pelo pool.

Referência de emissão: https://docs.godotengine.org/en/stable/tutorials/shaders/shader_reference/spatial_shader.html

## Geração

Os túneis ficaram aproximadamente 7–10 blocos largos e 6–8 altos, mantendo
curvas, ramificações, teto e bedrock. Os depósitos subterrâneos são bolsões
compactos, determinísticos pela seed e pelas coordenadas mundiais. Ferro fica
mais comum e o carvão aparece em profundidades amplas. Diamante continua raro
e profundo. Terra forma pequenos trechos nas paredes, chão e teto onde um
bolsão encontra o túnel. A camada geológica troca somente rocha sólida, sem
fechar passagens ou preencher ar.

Terreno é regenerado ao carregar e recebe as edições salvas depois. Um mundo
novo é a melhor forma de ver a distribuição sem interferência de minerações
anteriores. Minérios e pedra podem ser inspecionados imediatamente no inventário.

## Verificação

Compilação SCons, importação do Godot e os testes abaixo passaram. O teste de
integração carrega um mundo real em modo headless e confirma túneis, ferro,
carvão e terra em uma região subterrânea de 262.144 blocos. Testes das máscaras
confirmam que só as inclusões coloridas recebem emissão. Verificação headless
não avalia o halo final de glow em um monitor com renderização gráfica.

```sh
c++ -std=c++17 -O2 -Wall -Wextra -pedantic tests/underground_deposits_test.cpp -o /tmp/underground_deposits_test
/tmp/underground_deposits_test
c++ -std=c++17 -O2 -Wall -Wextra -pedantic tests/cave_tunnels_test.cpp -o /tmp/cave_tunnels_test
/tmp/cave_tunnels_test
XDG_DATA_HOME=/tmp/godot-underground-emission-tests godot --headless --path project --log-file /tmp/godot-emission-tests.log --script res://tests/ore_emission_test.gd
XDG_DATA_HOME=/tmp/godot-underground-world-tests godot --headless --path project --log-file /tmp/godot-underground-world.log --script res://tests/underground_world_test.gd
```

O teste de mundo cria um save de teste. Use sempre uma pasta isolada para
`XDG_DATA_HOME`, como nos comandos acima, para manter os saves do jogador separados.
