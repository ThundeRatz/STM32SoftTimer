# STM32SoftTimer

Esse repositório contém uma biblioteca para lidar com timers feitos por software.

Essa biblioteca foi feita para ser utilizadas como submódulo no [STM32ProjectTemplate](https://github.com/ThundeRatz/STM32ProjectTemplate).

## Utilizando a biblioteca

Para utilizar a biblioteca é necessário inicializar o timer com a função:

```C
void soft_timer_init(TIM_HandleTypeDef* htim, uint32_t max_reload_ms);
```
Além disso, é necessário declarar a função de interrupção de quando ocorre o overflow do timer em hardware, tipicamente:

```C
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
    /**< Code */
}
```
Dentro dessa função de interrupção é necessário verificar qual instância de timer em hardware causou a interrupção e chamar a seguinte função:

```C
soft_timer_period_elapsed_callback();
```

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