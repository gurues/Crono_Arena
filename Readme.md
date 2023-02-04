# CRONÓMETRO DE ARENA ELECTRÓNICO

## OBJETIVO

Con este proyecto quería desarrollar mi conocimiento con pantallas LED MAX7219 y el Acelerómetro-Giroscopio MPU-6050 (GY-521). También diseñar y crear un Cronómetro "chulo" para usarlo cuando juego al ajedrez con mis hijos.

![Cronómetro de Arena Electrónico](/Ayuda/10_Cronometro-Arena.jpg)
___

## DESCRIPCIÓN DEL PROYECTO

Este proyecto desarrolla un Cronómetro de arena electrónico mediante un Acelerómetro-Giroscopio MPU-6050 (GY-521) y 2 display led MAX7219. Mediante un pulsador podemos configurar el tiempo a cronometrar, borrar dicho tiempo y detener y arrancar el Cronómetro si así lo deseamos. El control del Cronómetro se realiza con un Arduino Nano alimentado por 2 baterías 18650 que a su vez se pueden cargar para dar autonomía al Cronómetro. El valor de carga de las baterías se muestra en un display.

El funcionamiento es el siguiente:

Mediante el Acelerómetro-Giroscopio se determina la posición del Cronómetro de arena. Si la posición es acorde para que la "arena" descienda, el Cronómetro se pone en marcha con el tiempo configurado, inicialmente 30 segundos, apagando y encendiendo leds simulando granitos de arena. Si se pulsa el pulsador se detiene la cuenta del Cronómetro y se permite la actuación del MPU para que determine si el Cronómetro está en una nueva posición. Si se vuelve a pulsar el pulsador la cuenta se pone de nuevo en funcionamiento. Cuando la cuenta del Cronómetro está en funcionamiento el MPU está desactivado por lo que se puede mover el Cronómetro de Arena sin que suceda nada. Cuando el Cronómetro está detenido se enciende el led de la esquina superior del panel led superior.
El control del Acelerómetro-Giroscopio se realiza mediante interrupciones. Mediante la variable __repeticiones__ configuramos el número de veces que el resultado del cálculo de la posición se debe repetir para que este sea tomada por correcta.
Para modificar la configuración inicial de tiempo se debe poner el Cronómetro de Arena en "horizontal", observando que se encienden los 4 leds de las esquinas de cada led MAX7219. Los 4 led de las esquinas encendidos indican que se encuentra en modo configuración.

En modo configuración y mediante el pulsador se puede realizar las siguientes acciones:

* Pulsación simple. Se enciende todos los led de uno de los MAX7219 indicando los minutos de tiempo configurados en el Cronómetro de Arena. Si por ejemplo, se pulsa 3 veces el pulsador (pulsaciones cortas), se encenderá 3 veces todos los led de uno de los MAX7219 indicando que el tiempo a cronometrar es de 3 minutos.
* Pulsación larga. Se enciende todos los led del otro MAC7219 indicando que hemos borrado la configuración de tiempo volviendo a la inicial, 30 segundos.

Para la depuración de código he programado los siguientes DEBUG por el puerto serie:

```Arduino
// Comentar para anular el DEBUG serie -> (Por defecto desactivo: Muestra el paso por las funciones de control de los led MAX7219)
//#define ___DEBUG___
// Comentar para anular el DEBUG1 serie -> (Por defecto activo: MPU6050, tiempo de crono y configuración del pulsador)
#define ___DEBUG1___
```

__Solo se permite activo uno de los DEBUG__.

El ___DEBUG___ muestra el paso por las funciones de control de los led MAX7219 y una vez visto que funciona correctamente se debe desactivar ya que modificaría las constantes tiempo de calibración del Cronómetro.

El ___DEBUG1___ muestra el funcionamiento del MPU6050 y como detecta la posición del cronómetro, el estado y control del pulsador y el tiempo transcurrido del Cronómetro. Por defecto está activo y se usará para la calibración de las constantes de tiempo del Cronómetro.

Calibración de Cronómetro de Arena:
El Cronómetro inicialmente está configurado para una cuenta a tras de 30 segundos en posiciones "verticales" y 15 segundos en posiciones "horizontales". La calibración se realizará de la siguiente forma:

* La calibración se realiza mediante el ajuste de las variables: __c_ajuste_0__, __c_ajuste_1__, __c_ajuste__ y __c_Tiempo__.
  * __c_Ajuste_0__ es la constante de tiempo fina para ajustar el valor final de los 30 segundos. Inicialmente se pone a 0.
  * __c_Ajuste_1__ es la constante de tiempo fina para ajustar el valor final de los 60 segundos. Inicialmente se pone a 0.
  * __c_Ajuste__ es la constante de tiempo fina para ajustar el valor final de los minutos configurados. Inicialmente se pone a 0.
  * __c_Tiempo__ es la constante de tiempo basto entre led's para cumplir el tiempo programado del Cronómetro. Esta constante se
    calibra con Cronómetro en 30 segundos y con los ajustes finos iniciados en 0. Se ajustará la constante __c_Tiempo__ a un valor para conseguir que el tiempo con el Cronómetro de Arena en posición "vertical" sea lo más cercano a 30 segundos sin pasarnos de este valor.
  * Asignamos en __c_Ajuste_0__ los milisegundos que faltan para llegar a 30 segundos.
  * Configuramos el Cronómetro de Arena a 1 minuto y comprobamos los milisegundos que faltan para que se obtengan los 60 segundos.
    Esos milisegundos se asignarán a la constante __c_Ajuste_1__.
  * Configuraremos el Cronómetro de Arena a 2, 3 y 4 minutos. De estos minutos sacaremos la diferencia en milisegundos que hay que
    añadir para que el resultado del Cronómetro sea correcto. Este valor se lo asignaremos a la constante __c_Ajuste__.

```Arduino
//Ajuste para calibrar el Cronómetro
unsigned long  init_time = 0; // Inicio de la cuenta
unsigned long  end_time = 0;  // Fin de la cuenta
const unsigned long c_ajuste_0 = 3; // Ajuste de tiempo fino para el calculo de 30 segundos
const unsigned long c_ajuste_1 = 52; // Ajuste de tiempo fino para el calculo de 1 minuto
const unsigned long c_ajuste = 96; // Ajuste de tiempo fino para el cálculo de los minutos
const unsigned long  c_Tiempo = 234;       
                              // Ajuste de tiempo basto entre led para cumplir el tiempo programado del cronómetro reloj de arena
                              // Por Defecto 30 seg -> 234 Factor que equivale a 30 seg
unsigned long  ajuste = c_ajuste_0;  // Predeterminado 30 segundos
unsigned long  Tiempo = c_Tiempo;
```

En la carpeta AYUDA del proyecto se comparte el diagrama de conexionado, fotos, video, así como los diseños en 3D del Cronómetro de Arena.

___

### Material necesario

* [Arduino nano](https://es.aliexpress.com/item/1005002976480289.html?spm=a2g0o.order_list.order_list_main.15.243d194dRODDD0&gatewayAdapt=glo2esp)
* [MPU-6050 (GY-521)](https://es.aliexpress.com/item/4000587196703.html?spm=a2g0o.productlist.main.71.5db78df1QcsUYc&algo_pvid=a0fdffc6-42fb-4a78-af61-26088dcc367a&algo_exp_id=a0fdffc6-42fb-4a78-af61-26088dcc367a-35&pdp_ext_f=%7B%22sku_id%22%3A%2210000003439265581%22%7D&pdp_npi=2%40dis%21EUR%211.62%211.48%21%21%21%21%21%402100b78b16742558581002978d06fd%2110000003439265581%21sea&curPageLogUid=2PQtqykRBDeo)
* [2 Pantallas led MAX7219 FC16](https://es.aliexpress.com/item/1005001548587546.html?spm=a2g0o.order_list.order_list_main.143.243d194dRODDD0&gatewayAdapt=glo2esp)
* [2 Condensadores de 10V y 330microFaradios o superior](https://es.aliexpress.com/item/1005001924143243.html?spm=a2g0o.productlist.main.3.2f4820efeNVEQZ&algo_pvid=7f27d3b0-441b-4177-99c3-6c31f3aa2e58&algo_exp_id=7f27d3b0-441b-4177-99c3-6c31f3aa2e58-1&pdp_ext_f=%7B%22sku_id%22%3A%2212000018125798787%22%7D&pdp_npi=2%40dis%21EUR%211.47%211.47%21%21%21%21%21%402100baf316742562760434120d06d7%2112000018125798787%21sea&curPageLogUid=lgueGGUIuWtw)
* [1 pulsador](https://es.aliexpress.com/item/32920583051.html?spm=a2g0o.productlist.main.23.568b4081JJxYnW&algo_pvid=3756ed57-b353-4c51-bd42-05f856d4a17e&aem_p4p_detail=202301201507152025122462612100009038420&algo_exp_id=3756ed57-b353-4c51-bd42-05f856d4a17e-11&pdp_ext_f=%7B%22sku_id%22%3A%2266006006964%22%7D&pdp_npi=2%40dis%21EUR%214.14%213.6%21%21%21%21%21%402100b69816742560353394599d0730%2166006006964%21sea&curPageLogUid=78Mjx81PbsAL&ad_pvid=202301201507152025122462612100009038420_12&ad_pvid=202301201507152025122462612100009038420_12)
* [1 interruptor](https://es.aliexpress.com/item/1005002279520828.html?spm=a2g0o.productlist.main.91.a7804324EZPAQP&algo_pvid=2ebd1f4d-6d72-4468-895f-9300f70160e5&aem_p4p_detail=202301201506013244155498178920008752143&algo_exp_id=2ebd1f4d-6d72-4468-895f-9300f70160e5-45&pdp_ext_f=%7B%22sku_id%22%3A%2212000019892827130%22%7D&pdp_npi=2%40dis%21EUR%212.67%211.81%21%21%21%21%21%402100ba4716742559619511698d075c%2112000019892827130%21sea&curPageLogUid=KeAbnDwnu1cY&ad_pvid=202301201506013244155498178920008752143_46&ad_pvid=202301201506013244155498178920008752143_46)
* [2 pilas 18650](https://es.aliexpress.com/item/1005004797151378.html?spm=a2g0o.productlist.main.25.2fff366fQENlg9&algo_pvid=d4a9aea5-4fb5-49bc-9582-da81623f17be&aem_p4p_detail=202301201509341916692496068800009044849&algo_exp_id=d4a9aea5-4fb5-49bc-9582-da81623f17be-12&pdp_ext_f=%7B%22sku_id%22%3A%2212000030521640624%22%7D&pdp_npi=2%40dis%21EUR%216.93%212.91%21%21%21%21%21%402100baf316742561743533620d06d7%2112000030521640624%21sea&curPageLogUid=fJyKtH12Q2yF&ad_pvid=202301201509341916692496068800009044849_13&ad_pvid=202301201509341916692496068800009044849_13)
* [Circuito de carga 18650 2S](https://es.aliexpress.com/item/1005004284015321.html?spm=a2g0o.order_list.order_list_main.68.243d194dRODDD0&gatewayAdapt=glo2esp)
* [Led de carga actual batería](https://es.aliexpress.com/item/33003710412.html?spm=a2g0o.order_list.order_list_main.44.243d194dRODDD0&gatewayAdapt=glo2esp)
* [Porta baterías 18650 2S](https://es.aliexpress.com/item/4000334921960.html?spm=a2g0o.order_list.order_list_main.62.243d194dRODDD0&gatewayAdapt=glo2esp)
  
___

__Mejoras y fallos del proyecto__:

El uso del pulsador para arrancar/parar el Cronómetro una vez iniciada la cuenta no es muy preciso. A veces es necesario pulsarlo varias veces o mantenerlo pulsado para que lo detecte. Esto es debido a que que usa la libreria "Switch.h" para el control del pulsador y se tiene que estar ejecutando constantemente la siguiente instrucción para que funcione:

```Arduino
PulsadorSwitch.poll();
```

Inicialmente solo quería el pulsador para la configuración de los minutos del Cronómetro por por lo que me decante por el uso de esta librería en vez de utilizar interrupciones para el control del pulsador. Posteriormente, quise utilizar este Cronómetro de arena como reloj para jugar al ajedrez y use el pulsador para arrancar/parar el Cronómetro. No puedo configurarlo con interrupciones ya que lo tengo montado y el cierre de la carcasa del Cronómetro lo realizo con pegamento. Pero como comento, si realizamos el control del pulsador con interrupciones los fallos y el retraso con algunas pulsaciones desaparecería. Queda para una próxima revisión del proyecto.

___

## Realizado por gurues (gurues@3DO ~ Eneno 2023 ~ ogurues@gmail.com)
