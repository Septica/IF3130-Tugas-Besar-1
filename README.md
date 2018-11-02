IF3130-Tugas-Besar-1
Lossless Data Transfer

# Cara Penggunaan

1. Untuk melakukan compilation dan membentuk binary file yang dibutuhkan, masukkan "make build" pada terminal.
2. Jalankan RecvFile pada satu terminal dengan cara mengetikkan 'make run-receiver output="&lt;Filename&gt;" ws="&lt;Windowsize&gt;" bs="&lt;Buffersize&gt;" port="&lt;Port&gt;"'.
3. Jalankan SendFile pada terminal lainnya dengan cara mengetikkan 'make run-sender input="&lt;Filename&gt;" ws="&lt;Windowsize&gt;" bs="&lt;Buffersize&gt;" address="&lt;DestinationIP&gt;" port="&lt;DestinationPort&gt;"'.
<br/>
P.S. filename pada SendFile adalah nama file yang akan dikirim misalkan "send.txt" yang disimpan pada folder sample. Sedangkan filename pada RecvFile adalah nama file yang akan diterima misalkan "received.txt" yang akan disimpan pada folder result   

# Cara Kerja Sliding Window dalam Program
1. SendFile
    - Menginisialisasi socket sesuai dengan parameter yang dimasukkan
    - Terdapat 2 thread, child thread akan melakukan handling masalah penerimaan ACK, parent thread akan melakukan pengiriman packet ke host lain dan sliding.
    - Child thread yang melakukan penerimaan ACK, akan menandai packet pada window yang sudah di-acknowledge. 
    - Thread kedua (parent thread) menghitung banyaknya slide yang dilakukan pada window. Banyaknya dihitung dari jumlah packet yang sudah di-acknowledge dari kiri window ke kanan. 
    - Setelah itu, main thread akan mengirim packet ke host yang dituju. Packet akan dikirim jika packet belum pernah dikirim atau sudah timeout tetapi belum mendapatkan ACK.
    - Setelah dikirim, packet akan ditandai, dan menambahkan timeout ke packet tersebut.
    - Sendfile akan langsung berhenti setelah menerima semua ack atau tidak mendapat respon balik dari server dan telah mengirim semua paket.

2. RecvFile
    - Menginisialisasi socket sesuai dengan parameter yang dimasukkan
    - Terdapat 2 thread, child thread akan melakukan penerimaan packet, packet akan dicek checksumnya, panjang datanya, lalu jika nomor packet yang diterima di bawah dari batas maksimum nomor window, maka ACK akan dikirim. Jika ada di dalam window akan dituliskan ke buffer dan ditandai.
    - Sedangkan parent thread melakukan penulisan ke file dan slding pada window. Jika terdapat shift yang dilakukan, akan file yang diterima akan ditulis.
    - Recvfile akan langsung berhenti setelah memproses packet terakhir. Packet terakhir didefinisikan sebagai packet yang memiliki data length yang lebih kecil dari length maximum, atau packet terakhir yang diterima setelah timeout.

# Pembagian Tugas
1. Felix Septianus - 13516041: SendFile
2. Dicky Adrian - 13516050: RecvFile
3. Cornelius Yan - 13516113: Packet, ACK, Makefile
