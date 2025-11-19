# Асинхронный сервер на базе epoll
## Установка и сборка
### Makefile
``` 
git clone https://github.com/JOKKEU/async-server && sudo apt install make && cd async-server/server && make
```
### Пакетом .deb
```
git clone https://github.com/JOKKEU/async-server && cd async-server && sudo dpkg -i server-pkg.deb
```
## Описание
Асинхронный сервер на основан на технологии epoll, работает как с tcp, так и с udp пакетами. 

Имеет консоль администрирования, где можно посмотреть, какие клиенты сейчас в онлайн, историю запросов, историю клиентов, активные айпи в локальной сети (которые откликнулись на пинг) в определенном диапазоне.

**Все функции**

<img width="812" height="539" alt="image" src="https://github.com/user-attachments/assets/ca9ac030-6379-42cb-a632-e806569b0317" />



**Запросы от клиентов:**
Реализованно 3 функции для ответа клиенту.
1) /time - выводит текущее время сервера.
2) /stats - выводит кол-во клиентов подключенных к серверу.

<img width="817" height="655" alt="image" src="https://github.com/user-attachments/assets/bf59e76f-15e6-4b0c-81fa-f235893aa3aa" />


**Как это выглядит на сервере**

<img width="981" height="286" alt="image" src="https://github.com/user-attachments/assets/b79811d5-6821-4a4c-93aa-c52558cc9a9d" />

**Так же опции консоли на сервер: просмотр активных айпи и время работы сервера:**

<img width="981" height="233" alt="image" src="https://github.com/user-attachments/assets/e4ef58f6-f4d8-47e6-beb1-1e213dfc3dd0" />

3) /shutdown - выключает сервер (при отключении, сервер сохраняет дамп).

**Как это выглядит:**

<img width="981" height="209" alt="image" src="https://github.com/user-attachments/assets/13ea97d6-a99d-4058-bcae-b08ef8ed1500" />

**Сторона сервера**

<img width="981" height="40" alt="image" src="https://github.com/user-attachments/assets/96108a2f-9b47-49d9-a464-7989b8849981" />

**Просмотр Дампа**

<img width="1200" height="630" alt="image" src="https://github.com/user-attachments/assets/125ab6cd-ab38-4a9d-9673-285701115728" />




## Особенности
Может не работать если клиент использует безпроводную сеть, а сервер проводную. Есть вероятность, что роутер заблокирует пакет, сервер просто не нейдет клиента.

Сервер может работать через broadcast, если роутер пропускает такие пакеты, иначе приходится вводить адрес сервера.
