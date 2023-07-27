# Hướng dẫn sử dụng Chat Group
1. Make
2. Tạo 4 terminal, trong đó:
    - Terminal 1: cd test-ser -> ../build/server <port>
    (trong dir test-ser sẽ chứa các file để client download hoặc chứa các file do client upload)
    - Terminal 2: ./run_multiclient.sh để tạo ̀500 clone clients (với mỗi client có tên là random_number và chờ để nhận message từ các clients khác)
    - Terminal 3: cd client -> ../build/client <port>
    (Client này được dùng để gửi tin nhắn cho các clone clients ở trên và test các command: "/upload <file-path>" hay "/download <file-name>" với file name là tên các file có trong dir test-ser)
    - Terminal 4: cd client -> ../build/client <port>
    (Tương tụ với client ở terminal , client ở đây dùng để test chat + /upload + /download)