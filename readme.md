## Roteador UDP L4.5 multicaminhos
Trabalho desenvolvido para a disciplina CI366 - Gerência de Redes - 1o Semestre 2019 - UFPR

Autores: 
  - Paolo Rechia
  - Eric Lowz

### Instalação, compilação e execução

É requisito possuir a versão 3.29 do NS3 instalado. 


Para instalar o modelo e o script de teste, basta:

```
cp -r src/applications/* [caminho_instalacao_ns3]/ns-allinone-3.29/ns-3.29/src/applications/
cp udp_multipath_router_test.cc [caminho_instalacao_ns3]/ns-allinone-3.29/ns3-29/scratch

```

Para compilar e executar o programa com os logs que exibem as tabelas do roteador:
```
cd [caminho_instalacao_ns3]/ns-allinone-3.29/ns3-29/
export NS_LOG=UdpMultipathRouterApplication=level_info
./waf --run scratch/udp_multipath_router_test
```
