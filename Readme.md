# Chat Group
1. Make 
2. ./server port-number
3. Mở thêm n terminal khác, mỗi terminal chạy ./client port-number

## Giải thích
1. Server
    - Định nghĩa 1 struct để lưu trữ thông tin của 1 client bao gồm socketfd và client name.
    - Tạo 1 mảng clients để lưu trữ thông tin các client.
    - Init mảng clients với mỗi client có sockfd = -1 và name = NULL.
    - Dùng vòng lặp while(1) để luôn listen trên port-number.
    - Mỗi khi có client kết nối đến server, dùng biến client_sockfd để lưu trữ giá trị của hàm accept và thêm giá trị này vào mảng clients ở trên.
    - Tạo 1 thread mới để xử lý connection này:
        + Nhận client_name bằng hàm recv().
        + Dùng vòng lặp do...while để nhận tin nhắn từ client và chuyển tiếp đến các clients đang kết nối đến server (sử dụng sockfd của mỗi client được lưu trong mảng clients).
2. Client
    - Kết nối đến server bằng hàm connect
    - Tạo 2 thread để gửi và nhận tin nhắn đồng thời:
    - thread gửi: send_message()
        + yêu cầu user nhập vào 1 tên không rỗng -> gửi đến server bằng hàm send()
        + dùng vòng lặp while(1) để liên tục gửi tin nhắn đến server
    - thread nhận: recv_message()
        + dùng vòng lặp while(1) để liên tục nhận tin nhắn từ server, in ra màn hình