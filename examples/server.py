import zmq
from colorama import Fore,init

init(autoreset=True)
context=zmq.Context()
socket=context.socket(zmq.REP)
socket.bind("tcp://0.0.0.0:2222")

print(Fore.YELLOW+"сервер ожидает подключений на порту 2222...")
counter=0


while True:
    data=socket.recv_string()
    counter+=1

    print(Fore.GREEN+f"Установлено подключение")
    print(Fore.BLUE+f"Получены данные {data}")

    with open("data.json","a",encoding="utf-8") as f:
        f.write(data+"\n")

    response=f"Получено пакетов: {counter}"
    socket.send_string(response)

    print(Fore.RED+f"Сервер отправил сообщение")
    print()


socket.close()
context.term()