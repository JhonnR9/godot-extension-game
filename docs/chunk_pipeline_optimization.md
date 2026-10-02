# Otimização do pipeline de chunks

Implementação de 02/10/2026, após a investigação em
[chunk_performance_review.md](chunk_performance_review.md).

## Mudanças aplicadas

- Resultados em `std::deque`, sem reconstrução da fila inteira. Recursos do
  resultado anterior são liberados fora do mutex de publicação.
- Scheduler compartilhado pelo gerador de modelos e pelo mesher: uma execução
  por posição, substituição do pedido pendente e revisão monotônica. Atualizações
  de vizinhos/AO também geram novas revisões. Resultados antigos não são instalados.
- Limite padrão de oito chunks em execução ou aguardando consumo **por etapa**.
  A fila de pedidos é deduplicada por posição; pedidos distantes são cancelados
  quando o foco muda. Jobs de modelo já iniciados podem terminar e ser aproveitados
  se o jogador voltar; jobs de mesh invalidam sua revisão quando saem da área.
- Tarefas finitas com lotes configuráveis de 1 a 8. O pool do Godot continua
  compartilhado com outras tarefas; nenhum worker fica bloqueado esperando pedidos.
- Workers geram apenas geometria. Criação e upload de ArrayMesh, instalação de
  colisões e configuração dos nós ocorrem na thread principal. Isso eliminou
  erros de RID encontrados ao criar meshes nos workers durante testes headless
  com vários mundos ativos.
- Finalização com orçamento padrão de 2 ms e teto de 32 resultados por frame.
  Uma instalação individual pode ultrapassar o orçamento: o tempo é verificado
  entre resultados. O pico é exposto nas estatísticas.
- Metadados dos blocos carregados uma vez por instância do mundo, em tabelas
  imutáveis indexadas por ID/face. Workers não abrem JSON nem carregam texturas.
- Removidos placeholders que alocavam 256 KiB por posição apenas para informar
  que a geração estava pendente.
- Cache limitado a 256 colunas de chunks XZ, por configuração/mundo. Os mesmos
  dados de terreno são reaproveitados entre diferentes alturas Y. Amostragem
  de ruído ocorre fora do mutex do cache.
- Busca dos 27 vizinhos em uma chamada ao repositório. Edições copiam um chunk
  apenas quando existem referências externas ao repositório; snapshots retidos
  por jobs preservam os blocos anteriores. Sem leitores externos, edição é local.
- Contagem de blocos não vazios permite pular o mesher de chunks de ar.
  Varredura de plantas usa X como eixo interno, acompanhando o layout dos voxels.
  Máscaras de faces são reutilizadas dentro de cada tarefa/lote.
- Streaming solicita um halo de dados, incluindo diagonais, além da área visível.
  A mesh espera esse halo para evitar AO temporário nas bordas. As tentativas
  ocorrem por mudança de foco, chegada de modelo ou edição, em vez de varrer o
  mundo ativo em todos os frames.
- Rebuilds atualizam o ChunkNode existente; tochas cujas posições não mudaram
  preservam suas luzes. A finalização usa a classificação de superfícies recebida
  com a geometria, sem recuperar arrays de uma mesh para identificar água.
- TaskIDs são recolhidos e aguardados na troca/fechamento de mundo; filas, edições
  e dados antigos são limpos antes de alterar seed/ruídos/repositório. O pool de
  nós respeita o teto de prewarm e pode crescer conforme a demanda real.

O halo tem um custo de memória: no cenário de raio 4 e altura 2, são 623 modelos
para 245 posições visíveis. Só os blocos desses modelos ocupam aproximadamente
155,75 MiB, fora meshes e caches. A remoção dos placeholders evita uma alocação
redundante, mas não garante menor memória total que o motor anterior, que deixava
bordas sem os vizinhos necessários. Compactar dados uniformes ou armazenar apenas
as faixas necessárias do halo é uma otimização futura.

## Configuração e medições

Depois de adicionar VoxelAPI à árvore:

```gdscript
world.set_pipeline_settings({
    "batch_size": 1,
    "max_inflight": 8,
    "finalize_budget_ms": 2.0,
})
var stats: Dictionary = world.get_pipeline_stats()
```

`batch_size` é limitado a 1–8, `max_inflight` a 1–16 por etapa e o orçamento a
0,1–8 ms. Reduzir o limite não interrompe jobs já iniciados; eles terminam antes
que novas submissões utilizem a capacidade reduzida.

As estatísticas incluem pedidos pendentes, chunks em execução/aguardando consumo,
resultados prontos, pico de fila, tarefas submetidas, resultados obsoletos,
tempo médio/máximo de trabalho, espera média desde o pedido, tempos cumulativos
segurando/esperando o mutex da fila e duração da finalização. A espera inclui a
fila local e, em lotes, os chunks anteriores da tarefa. Leituras de estatísticas
copiam os contadores sob lock e constroem o Dictionary depois de soltá-lo.

O tempo de trabalho não inclui a instalação visual/física na thread principal.
As estatísticas reiniciam na troca de mundo. A thread principal continua sendo a
única dona dos estados de submissão, revisões, estágios e flags dos chunks.

## Comparação de lotes

O teste reproduzível está em `project/tests/chunk_pipeline_benchmark.gd`; os
resultados completos ficam em [chunk_batch_benchmark.json](chunk_batch_benchmark.json).
Mesma seed 12345, foco, cobertura física, raio 4, altura 2, limite 8 por etapa e
orçamento 2 ms. Três execuções por tamanho de lote, build `template_debug`,
headless e limite de 120 frames/s. O cronômetro começa em `start_world`, depois
que o mundo foi adicionado à árvore e o pool foi preparado.

A comparação mede o carregamento inicial da região central e a drenagem de todos
os pedidos da área e do halo. Não mede FPS/GPU nem compara diretamente com o
motor anterior. A ordem de chegada dos chunks pode mudar a quantidade de
trabalhos que precisam de atualização; por isso as estatísticas brutas acompanham
os tempos. Os limites de frames também afetam a frequência de despacho/consumo.

| Lote | Carga central (mediana) | Drenagem total (mediana) | Submissões de modelos | Submissões de meshes (mediana) |
|---:|---:|---:|---:|---:|
| 1 | 280.4 ms | 780.5 ms | 623 | 245 |
| 2 | 242.9 ms | 818.1 ms | 312 | 147 |
| 4 | 276.2 ms | 884.7 ms | 156 | 93 |
| 8 | 301.4 ms | 934.9 ms | 78 | 75 |

Uma tarefa por chunk permanece como padrão por oferecer a drenagem total mais
rápida neste teste. Lote 2 entregou a região central mais cedo, portanto pode ser
preferível para a carga inicial. O limite é contado em chunks: com limite 8, lote
8 pode usar apenas uma tarefa por etapa, reduzindo paralelismo. Lotes
maiores ficam disponíveis para medir em builds release e em outras máquinas;
reduzir submissões não implica reduzir o tempo até um chunk ficar visível.

## Validação

- Build da extensão: `scons -j6 target=template_debug`.
- `chunk_pipeline_test.gd`: limites das filas, amortização de submissões com lote
  2, halo, mundo parado, última revisão após edições em canto, preservação do
  nó/luz, retorno rápido do foco, salvamento/reload, troca e destruição com jobs
  pendentes.
- `torch_test.gd`: colocação, ícone, luz, seleção, reload e remoção da última tocha.
- `biome_registry_test.gd`: geração/sampler, árvores em diferentes ordens de
  carregamento, limites de altura e bedrock. As fixtures foram ajustadas às
  dimensões 32×64×32 que já existiam no workspace.
- `voxel_ao_test.gd` com renderer OpenGL Compatibility: seis faces, diagonal entre
  chunks, transparência e greedy meshing. As coordenadas de borda da fixture
  também foram ajustadas às dimensões atuais.

O tamanho 32×64×32 e o formato de saves foram preservados. Alterar dimensões,
separar seções de mesh/colisão, comparar tarefas de grupo e implementar LOD
continuam sendo experimentos separados. Não há afirmação de ganho de FPS sem
comparação em build release com o renderer e a rota do jogo.
