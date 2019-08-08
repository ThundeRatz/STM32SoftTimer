# STM32SoftTimer

Esse repositório contém uma biblioteca para lidar com timers feitos por software.

Essa biblioteca foi feita para ser utilizadas como submódulo no [STM32ProjectTemplate](https://github.com/ThundeRatz/STM32ProjectTemplate).

## Adicionando o submódulo ao projeto

Crie um diretório chamado `lib`, caso não exista:

```bash
mkdir lib
```
E adicione o submódulo fazendo:

* Com HTTPS:
```bash
git submodule add --name STM32SoftTimer https://github.com/ThundeRatz/STM32SoftTimer.git lib/STM32SoftTimer
```

* Com SSH:
```bash
git submodule add --name STM32SoftTimer git@github.com:ThundeRatz/STM32SoftTimer.git lib/STM32SoftTimer
```

---------------------

Equipe ThundeRatz de Robótica