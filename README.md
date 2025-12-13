
# Projeto de Módulo de Kernel (LKM)

Este projeto contém um **módulo de kernel**.  
Usamos **CMake** apenas para organizar o projeto e gerar suporte ao editor (LSP/clangd), enquanto a **compilação real é feita pelo Make/Kbuild**.

---

## Estrutura do projeto

```

.
├── build/                # Diretório de build
├── CMakeLists.txt        # Arquivo CMake principal
├── headers/              # Headers compartilhados
├── kernel/
│   ├── Makefile          # Kbuild
│   └── my_driver.c       # Código do módulo
├── LICENSE
└── README.md

````

---

## Pré-requisitos

- Linux com headers do kernel instalados:

```bash
sudo apt install build-essential linux-headers-$(uname -r)
````

* CMake 3.16 ou superior
* make

---

## Compilando o módulo

1. Crie o diretório de build e entre nele:

```bash
mkdir -p build
cd build
```

2. Rode o CMake para gerar os targets:

```bash
cmake ..
```

> Isso cria o target `kernel_module` e também `kernel_module_clean`.
> O `kernel_module` já é marcado como `ALL`, então **apenas `make` já compila o módulo**.

3. Compile o módulo:

```bash
make
```

> O arquivo resultante será `kernel/my_driver.ko`.

---

## Limpando o build

Para limpar arquivos gerados pelo kernel:

```bash
make kernel_module_clean
```

> Remove `.ko`, `.o` e outros arquivos intermediários.

---

## Carregando e descarregando o módulo

```bash
# Carregar o módulo
sudo insmod kernel/my_driver.ko

# Ver mensagens do kernel
dmesg | tail

# Descarregar o módulo
sudo rmmod my_driver
```

---

## Integração com IDE / LSP (opcional)

Se você usa **clangd** ou VSCode:

* CMake gera automaticamente `compile_commands.json` no diretório `build/`.
* Isso permite:

  * Autocompletar
  * Navegação entre headers
  * Diagnóstico em tempo real

Basta abrir o diretório `build/` no editor.

---

## Observações

* Todo o código do módulo fica em `kernel/`.
* Headers compartilhados ficam em `headers/`.
* O CMake **não compila o módulo diretamente**, ele apenas organiza a execução do Make/Kbuild e facilita o LSP.





