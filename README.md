# IF3130-Tugas-Besar-1
Lossless Data Transfer

# Cara Penggunaan

1. Jalankan makefile, dengan cara mengetikkan './makefile.sh' pada terminal
2. Jalankan RecvFile pada satu terminal dengan cara mengetikkan './RecvFile <Filename> <Windowsize> <Buffersize> <Port>'.
3. Jalankan SendFile pada terminal lainnya dengan cara mengetikkan './SendFile <Filename> <Windowsize> <Buffersize> <DestinationIP> <DestinationPort>'.
P.S. filename pada SendFile adalah nama file yang akan dikirim misalkan 'send.txt' yang disimpan pada folder sample. Sedangkan filename pada RecvFile adalah nama file yang akan diterima misalkan 'received.txt' yang akan disimpan pada folder result   

# Cara Kerja Sliding Window dalam Program


#Pembagian Tugas