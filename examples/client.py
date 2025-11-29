import socket

client_socket=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
client_socket.connect(('localhost',666))

while True:
    data=input("Введите данные для отправки: ").encode()
    client_socket.sendall(data)

    answer = client_socket.recv(1024).decode()
    print(f"Данные от сервера: {answer}")


client_socket.close()