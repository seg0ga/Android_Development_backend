import socket
from colorama import Fore

server_socket=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
server_socket.bind(('localhost',666))

server_socket.listen(1)
print(Fore.YELLOW+"Сервер ожидает подключения...")
client_socket,client_address = server_socket.accept()
print(Fore.GREEN+f"Установлено подключение с {client_address}")


while True:
    data=client_socket.recv(1024).decode()
    print(Fore.BLUE+f"Получены данные: {data}")

    client_socket.sendall(f"Cообщение \"{data}\" успешно получено".encode() )
    print(Fore.RED+f"Сервер отправил сообщение {client_address}")

client_socket.close()
server_socket.close()