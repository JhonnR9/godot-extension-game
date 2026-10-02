# Cache de ícones dos blocos

O autoload `BlockIconCache` prepara os ícones antes de abrir o menu. A primeira
execução gera PNGs RGBA de 64 × 64 em `user://cache/block_icons`. As próximas
execuções carregam esses PNGs como `ImageTexture`, compartilhados pelo
inventário criativo, hotbar e preview de arrastar itens.

A composição usa projeção de três faces conforme `visual_bugs/images.png`,
textura superior e lateral do registro gerado, tint do bloco e sombreamento
por face. A amostragem é nearest, com fundo transparente. Plantas com flag
`crossed` mantêm sua silhueta. Água usa uma cor própria porque sua aparência
no mundo vem de um shader procedural.

Não depende de renderizar uma cena 3D nem de ler pixels da GPU: a composição
usa imagens dos recursos Texture2D importados, inclusive em builds exportadas.
Na execução com cache válido, não carrega as imagens-fonte para composição.

`manifest.cfg` registra uma assinatura SHA-256 por bloco, combinando versão
do gerador, conteúdo das texturas importadas e metadados do registro. Arquivos
ausentes, inválidos ou de dimensão errada são regenerados. Mudanças no registro
invalidam os blocos afetados; mudanças nas texturas invalidam os ícones para
que não fiquem desatualizados. PNGs e manifesto são escritos em arquivos
temporários antes de substituição. Se o disco não permitir gravar, os ícones
continuam disponíveis em memória, mas serão gerados novamente no próximo início.

O diretório físico depende da plataforma e da configuração `user://` do Godot.
Apagar esse cache faz o jogo reconstruí-lo na próxima abertura.

Teste sem alterar o cache normal do jogador:

```sh
XDG_DATA_HOME=/tmp/godot-block-icon-tests godot --headless --path project --log-file /tmp/godot-icons-test.log --script res://tests/block_icon_cache_test.gd
```

O teste verifica geração inicial, reutilização com uma instância nova,
recuperação de arquivos ausentes ou com dimensão inválida, mudança de tint,
projeção e integração com o inventário. `last_run` expõe os contadores
`generated` e `loaded` para verificar execuções independentes.
