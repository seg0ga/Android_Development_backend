import zmq
from colorama import Fore, init
import json
import os

init(autoreset=True)


def print_data():
    all_locations=[]
    if os.path.exists("data.json"):
        with open("data.json","r",encoding="utf-8") as f:
            for line in f:
                line=line.strip()
                if line:
                    location=json.loads(line)
                    all_locations.append(location)

    print(Fore.GREEN+f"\nВсего записей: {len(all_locations)}")
    for i,location in enumerate(all_locations,1):
        print(f"{i}. lat: {location['lat']}, lon: {location['lon']}, alt: {location['alt']}, time: {location['time']}")


def clear_data():
    if os.path.exists("data.json"):os.remove("data.json")
    print(Fore.GREEN+"Данные очищены")


def start_server():
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

        json_data=json.loads(data)
        locations=json_data["locations"]

        with open("data.json","a",encoding="utf-8") as f:
            for location in locations:
                f.write(json.dumps(location,ensure_ascii=False)+"\n")

        response=f"Получено пакетов: {counter}"
        socket.send_string(response)

        print(Fore.RED+f"Сервер отправил сообщение")
        print()
    socket.close()
    context.term()

while True:
    print(Fore.YELLOW+"\nМЕНЮ:")
    print("1. Запустить сервер")
    print("2. Показать данные")
    print("3. Очистить данные")
    print("4. Выход")

    choice=input("Выберите: ")

    if choice=="1":start_server()
    elif choice=="2":print_data()
    elif choice=="3":clear_data()
    elif choice=="4":break
    else:print(Fore.RED+"Неверный выбор")