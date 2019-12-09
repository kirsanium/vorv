### Build
`mpicxx main.cpp`

### Run
`mpirun -n 5 ./a.out input.txt`

### input.txt format
1st line: `n`, where n = number of slave processes required
2...nth line: `x y` where x = number of process to print message (1...n), y = message

### Примечания
В коде есть строка, выводящая информацию о процессе перед вызовами MPI_Barrier() и MPI_Finalize(), после которой иногда крашится программа (она крашится и без этого принта).
