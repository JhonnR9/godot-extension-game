# Investigação de desempenho dos chunks

As melhorias foram implementadas posteriormente; veja [chunk_pipeline_optimization.md](chunk_pipeline_optimization.md). Este documento registra a investigação anterior às mudanças.

Data: 02/10/2026. Análise do código atual do workspace, incluindo alterações
locais que já existiam. Nenhum comportamento do motor foi modificado nesta
investigação. Foi acrescentado um microbenchmark reproduzível.

## Conclusão

A primeira melhoria deve ser no caminho entre produção e consumo: eliminar a
reconstrução integral da fila de meshes, impedir trabalho duplicado, encurtar as
seções críticas e limitar a instalação por tempo. Mudar o tamanho dos chunks ou
agrupar muitas gerações numa tarefa antes disso pode deslocar o gargalo para a
thread principal e aumentar a latência das edições.

São custos e riscos confirmados pela leitura do código. O peso relativo de
geração, meshing, scheduler, renderer e física ainda precisa de instrumentação
no jogo; não houve medição de FPS, contenção real ou tempo de tarefas nativas.

## Achados e propostas

### 1. Consumir poucas meshes custa proporcionalmente à fila inteira

Em `src/chunk_mesh_async_generator.cpp:65`, consumir K resultados percorre N
resultados e recria dois HashSets sob `_generated_meshes_mutex`. Os produtores
precisam desse mesmo mutex para publicar resultados. Uma drenagem completa em
pequenos lotes soma aproximadamente O(N²/K) trabalho de reconstrução.

A atribuição na linha 91 usa `std::move`, mas o HashSet deste checkout só tem
atribuição por cópia (`godot-cpp/include/godot_cpp/templates/hash_set.hpp:417`).
Logo, a fila restante é também copiada; seus dados não são simplesmente
transferidos. As cópias de resultados copiam referências, não toda a geometria,
mas repetem operações de referência, alocações e cópias das tabelas.

**Proposta:** `std::deque<MeshResult>` para resultados prontos; retirar K com
`pop_front` para um vetor local e finalizar fora do mutex. Custo O(K) por retirada.
Para consumo total, `swap` com uma fila local reduz ainda mais a seção crítica.
Destruir resultados obsoletos e recursos fora do lock. Manter conjuntos/maps
separados para presença, jobs em execução e versão desejada.

Deque resolve o transporte FIFO; prioridade por distância exige filas por
classe de prioridade ou outra estrutura. FIFO de conclusão sozinho não garante
que o chunk mais próximo seja instalado primeiro. Não substituir por deque os
HashSets de chunks ativos ou dirty: ali deduplicação e consulta de presença são
funções úteis.

### 2. Jobs duplicados e resultados novos perdidos

`queue_async_generate_mesh` insere a posição em `_generating_meshes` sem verificar
se ela já existe. `_rebuild_chunk` pode enviar outro job enquanto o anterior
ainda roda. O conjunto de resultados considera iguais duas meshes da mesma
posição, independentemente da versão. O HashSet retorna a entrada existente ao
reinserir a chave; não atualiza seu valor.

Se v1 terminar e permanecer na fila, e depois v2 terminar, v2 pode ser ignorada.
A finalização rejeita v1 pela versão atual, mas v2 já foi perdida. Além disso,
o primeiro job a acabar apaga a posição do conjunto mesmo que outro job da
mesma posição continue em execução. Portanto, o conjunto não representa a
quantidade real de trabalhos pendentes.

**Proposta:** um job em execução por posição, com revisão desejada e marcador de
nova solicitação. Edições durante a execução atualizam a revisão desejada; ao
terminar, publicar um resultado válido ou agendar uma única atualização.
Usar identidade do mundo/epoch e revisão de solicitação, além da versão dos
blocos. Alterações em vizinhos e no halo de AO também invalidam a mesh; a versão
do chunk central sozinha não cobre essas dependências.

### 3. Finalização pode produzir picos na thread principal

`src/voxel_api.cpp:228` escolhe 1 ou 2 meshes por frame normalmente, 5 quando há
mais de 200 resultados e **100** quando há mais de 1.000. Aumentar o trabalho
bruscamente justamente quando a fila cresce pode agravar os picos. O `delta`
representa o frame anterior e inclui outros custos; não mede a instalação atual.

`_finalize_chunk` instala recursos, busca arrays de superfícies, configura
colisão e luzes. `ChunkNode::set_collision_faces` chama `set_faces`; sua parcela
de custo ainda não foi medida. Num rebuild, remover o nó anterior limpa a
colisão e as luzes, e a instalação as recria.

**Proposta:** orçamento configurável por tempo, inicialmente experimentar
1–2 ms/frame de instalação, com limite adicional de quantidade. Verificar o
tempo entre resultados; uma única instalação pesada ainda pode ultrapassar o
orçamento. Atualizar o nó existente nos rebuilds. Transportar a classificação
das superfícies no resultado, evitando recuperar arrays para identificar água.
Priorizar colisões próximas ao jogador e avaliar instalação separada da visual.

### 4. Varredura e sincronização repetidas

`_update_visible_chunks` copia todos os chunks ativos e os percorre a cada frame.
Cada busca do repositório adquire seu mutex. `_get_neighbors_for` realiza 27
buscas, portanto 27 aquisições separadas por tentativa de mesh. Chunks em
`WAITING_NEIGHBORS` podem repetir essas tentativas em todos os frames.

No raio horizontal 4 e altura 3 existem 49 × 7 = **343** chunks ativos, longe
dos limites verticais do mundo. O tamanho cresce aproximadamente com R² × H.
O `cache_radius` não é usado pelo streaming para gerar uma camada extra de
vizinhos: os chunks das bordas podem ficar esperando vizinhos nunca solicitados
num mundo recém-criado. Isso também mantém tentativas inúteis a cada frame.

**Proposta:** gerar dados numa camada de halo além dos chunks visíveis; dirigir
as tentativas de mesh por eventos (chunk entrou, dados chegaram, vizinho chegou,
edição ocorreu), com deduplicação das posições pendentes. Buscar os 27 ponteiros
numa chamada ao repositório sob um único lock. Somente adotar isso após definir
como os dados permanecem consistentes durante o meshing.

### 5. Inicialização repetida por mesh

Cada job constrói `ChunkMeshBuilder`. Seu construtor solicita a textura pelo
ResourceLoader e abre/interpreta `block_registry.generated.json`, reconstruindo
maps de texturas e cores. O carregador pode reutilizar a textura em cache, mas
a abertura do JSON e sua interpretação são explícitas a cada construção.
`block_texture_array` não é usada por `build` no código atual.

**Proposta:** carregar metadados uma vez e compartilhar um snapshot imutável,
preferencialmente com tabelas indexadas por ID/face. Não compartilhar buffers
mutáveis do builder. Reutilizar scratch buffers por tarefa que processa um
pequeno lote, ou por worker com estratégia de reentrância bem definida.

### 6. Memória por chunk e acesso aos voxels

Os chunks atuais são **32 × 64 × 32 = 65.536 voxels**. `Block` tem 4 bytes:
**256 KiB** de blocos por chunk, além de estágio, flags e overhead da alocação.
O placeholder de geração também é um Chunk completo zerado, apesar de só servir
para informar o estágio. Para 343 posições, placeholders representam cerca de
85,75 MiB de blocos; os resultados gerados que aguardam consumo podem coexistir
com esses placeholders. Cada geração aloca ainda 64 KiB de prioridades de
escrita e os dados das 1.024 colunas, além dos buffers dos passes.

O mesher já usa greedy meshing. Ainda assim, executa seis varreduras de volume
para faces, varreduras de máscaras e outra de volume para plantas. Esta última
usa z como eixo interno, mas o layout é `x + y*SIZE_X + z*SIZE_X*SIZE_Y`: o passo
interno é de 8.192 bytes. Há uma oportunidade concreta de melhorar localidade
usando x como eixo interno. Nem todas as orientações de face terão localidade
ideal com o mesmo layout.

`BiomeSelectionPass` recalcula as mesmas 1.024 colunas XZ para cada chunk Y da
mesma coluna de chunks. O sampler chama seis recursos de ruído por coluna.

**Proposta:** placeholder leve (posição, estágio, revisão), cache imutável de
amostragem por coluna XZ e configuração do mundo, percursos contíguos quando
possível e indicadores de chunk vazio/ocupação para evitar meshing de ar.
Manter esses indicadores nas edições; um chunk sólido só pode pular faces se o
halo provar que elas não ficam expostas. Medir também o cache de árvores, que
faz consultas e atualizações LRU com mutex e copia candidatos.

### 7. Correção da sincronização antes de ampliar o paralelismo

Os jobs guardam ponteiros crus para os generators/loaders e ignoram o TaskID
retornado pelo WorkerThreadPool. Não há espera explícita pelo término dos jobs
antes de liberar seus donos. `_clear_world` está vazio. Uma mudança de mundo
pode misturar estado antigo e novo, além de deixar jobs antigos pendentes.

O mutex do repositório protege o map, mas o shared_ptr retornado não protege os
blocos: `set_block` escreve no mesmo Chunk que os workers leem no mesher.
O estágio do placeholder também é escrito pelo worker e lido fora do lock na
thread principal. Checar a versão depois não elimina essas corridas de dados.

**Proposta:** snapshots imutáveis para meshing, ou sincronização explícita de
leitura/escrita de blocos; ciclo de vida com TaskIDs, cancelamento lógico e
descarte por epoch. Ao fechar/trocar mundo, impedir novas submissões e garantir
que jobs não acessem objetos destruídos nem recursos de ruído reconfigurados.
Snapshots do chunk central + halo de um voxel podem evitar copiar 27 chunks
inteiros. Definir o custo e a consistência desse snapshot antes de implementá-lo.

## Uma tarefa por chunk ou lotes?

O código já usa o WorkerThreadPool global do Godot; não cria uma thread por
chunk. Atualmente cria **uma tarefa nativa por chunk**, tanto para modelo como
para mesh. Não há evidência medida de que o custo de submissão domine os passes.

Há três decisões distintas:

1. **Publicar/consumir em lote:** reduz aquisições do mutex e trabalho de fila;
   tem benefício estrutural claro. Publicar lotes pequenos evita reter resultados
   prontos por muito tempo.
2. **Submeter uma lista ao pool:** comparar a tarefa individual com a API nativa
   de grupos disponível nos headers locais. Um grupo pode distribuir índices
   dinamicamente entre workers, sem exigir que uma tarefa processe toda a lista.
3. **Cada tarefa executar K chunks sequencialmente:** experimentar K = 1, 2, 4,
   8. Amortiza submissão e preparação, mas lotes grandes pioram balanceamento,
   prioridade, cancelamento e tempo até o primeiro resultado. Terreno vazio,
   cavernas, vegetação e meshes de superfície têm custos distintos.

Minha proposta inicial é um número limitado de tarefas em execução, extraindo
pequenos lotes de uma fila prioritária e verificando cancelamento entre chunks.
Rebuilds próximos ao jogador devem usar lote pequeno. Não manter loops de worker
bloqueados indefinidamente dentro do pool compartilhado, que também atende IO e
outras tarefas. Aplicar limites à quantidade de resultados e memória pendentes;
apenas aumentar produtores pode acumular mais meshes do que a thread principal
consegue instalar.

## Avaliação do tamanho

| Dimensões | Voxels | Blocos/chunk | Chunks para o mesmo volume físico |
|---|---:|---:|---:|
| 16 × 32 × 16 | 8.192 | 32 KiB | 8 vezes o atual |
| 32 × 32 × 32 | 32.768 | 128 KiB | 2 vezes o atual |
| 32 × 64 × 32 (atual) | 65.536 | 256 KiB | referência |
| 64 × 64 × 64 | 262.144 | 1 MiB | 1/4 do atual |

Chunks menores podem diminuir o custo e a latência de um rebuild isolado, mas
aumentam jobs, nós, fronteiras, colisões e superfícies/draw calls potenciais.
Chunks maiores amortizam agendamento, mas tornam edições e uploads mais caros.

Primeiro comparar o atual com **32³**, depois **16 × 32 × 16**, mantendo a mesma
distância em metros e extensão vertical. Avaliar separar o tamanho de armazenamento
do tamanho das seções de mesh/colisão: dados maiores e rebuilds menores podem
ser uma alternativa à mudança global.

Não alterar só as constantes: a altura total do mundo depende de SIZE_Y, saves
guardam coordenadas de chunk/bloco local e árvores usam densidade/candidatos por
chunk. Preservar cobertura física, determinismo, densidade e compatibilidade
dos mundos exige mudanças coordenadas.

## Microbenchmark executado

`tests/chunk_queue_benchmark.cpp` usa o HashSet deste checkout e reproduz o
algoritmo de retirada atual, comparando-o com deque + vetor local. Registro
representativo com três shared_ptr, posição e versão; adaptadores de alocação
usam malloc/realloc/free. Sem instanciar recursos Godot. Sete repetições;
medianas. A preparação da fila fica fora do trecho cronometrado. Confere contagem
e soma dos IDs consumidos. CPU local: Intel Core i5-12400F; compilação `-O3`.

| Fila inicial | Lote | Primeira retirada, HashSet | Primeira retirada, deque | Drenagem completa, HashSet | Drenagem completa, deque |
|---:|---:|---:|---:|---:|---:|
| 200 | 2 | 5,126 µs | 0,048 µs | 0,304 ms | 0,002 ms |
| 1.000 | 2 | 54,595 µs | 0,052 µs | 7,431 ms | 0,011 ms |
| 10.000 | 2 | 771,457 µs | 0,059 µs | 894,443 ms | 0,115 ms |
| 1.000 | 100 | 44,614 µs | 0,728 µs | 0,212 ms | 0,008 ms |

Esses tempos avaliam a estrutura e o algoritmo de consumo. Não incluem geração,
meshing, física, GPU, custo real das referências Godot, concorrência de produtores
ou espera de mutex. A drenagem completa soma retiradas sem espaçá-las por frames.
Não são previsão de ganho de FPS. As medidas abaixo de um microssegundo têm
sensibilidade ao relógio, caches e escalonamento; a mudança de complexidade é a
evidência principal. A fila de 10.000 é um cenário de estresse, não uma fila
observada no jogo.

Reprodução, a partir da raiz do projeto:

```sh
g++ -O3 -DNDEBUG -std=c++17 -Igodot-cpp/include -Igodot-cpp/gen/include \
    -Igodot-cpp/gdextension tests/chunk_queue_benchmark.cpp -o /tmp/chunk_queue_benchmark
/tmp/chunk_queue_benchmark
```

## Ordem recomendada e medições necessárias

1. Instrumentar timestamps de solicitação, início/fim do job, publicação e
   instalação. Separar espera no pool, espera/tempo segurando mutex, geração por
   pass, metadados, meshing, criação da mesh e instalação visual/física. Medir
   também memória, tamanho máximo das filas, trabalhos descartados/duplicados e
   contagem de rebuilds por posição.
2. Corrigir versões, deduplicação, corridas e ciclo de vida; trocar a fila pronta
   por deque e encurtar o lock. Implementar orçamento temporal na finalização.
3. Compartilhar metadados, remover placeholders volumosos, agrupar busca de
   vizinhos e substituir varredura por eventos, com halo de dados definido.
4. Comparar tarefas individuais, grupos e lotes 2/4/8 com limite de pendências.
5. Só então comparar tamanhos e seções independentes de mesh.

Usar mesma seed, área em metros, configuração e rota; repetir cenários de carga
inicial, jogador parado, travessia de fronteiras, deslocamento rápido e edições
em faces/arestas/cantos. Registrar p50/p95/p99 do frame e da latência até render,
throughput de chunks e memória máxima em builds release. Validar o resultado
visual e colisões no renderer utilizado pelo jogo, incluindo AO, água, plantas
e tochas. Headless ajuda a isolar CPU, mas não valida o custo de GPU.
