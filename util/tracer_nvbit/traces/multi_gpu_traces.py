import os

for i in range(8):
    num_gpu = i+1
    dir = f'vector_add_gpu{num_gpu}'
    os.mkdir(dir)
    os.chdir(dir)
    for j in range(num_gpu):
        gpu_dir = f'GPU_{j}'
        os.mkdir(gpu_dir)
        file_name = f'kernel-{j}.traceg'
        os.system('echo `pwd`')
        os.system(f'cp ../GPU_0/kernel-1.traceg {gpu_dir}/{file_name}')
        os.system(f'echo {file_name} >> {gpu_dir}/kernelslist.g')
        os.system(f'echo {file_name} >> ./kernelslist.g')
    os.chdir('..')