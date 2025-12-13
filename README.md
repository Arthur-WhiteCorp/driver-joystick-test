# Linux Kernel Module – Driver de Joystick (Exemplo)

Este repositório contém um **módulo de kernel Linux (LKM)** desenvolvido para fins de estudo.  
O projeto já vem configurado para facilitar **compilação**, **carregamento** e **uso em editores** (VS Code / Neovim).

---

## 📋 Requisitos

- Linux (testado em Ubuntu)
- Headers do kernel instalados
- `make`
- GCC
- (Opcional, mas recomendado) `bear` para suporte a clangd

Para instalar os headers do kernel:

```bash
sudo apt install linux-headers-$(uname -r)
````

Para instalar o Bear (opcional):

```bash
sudo apt install bear
```

---

## 📁 Estrutura do projeto

```text
.
├── Makefile
├── compile_commands.json        (gerado automaticamente)
├── .clangd                      (configuração do clangd)
└── kernel/
    ├── testLKM.c                (código do módulo)
    ├── Makefile
    └── compile_commands.json -> ../compile_commands.json
```

---

## ⚙️ Como compilar o módulo

Na raiz do projeto, execute:

```bash
make
```

Isso irá:

* Compilar o módulo do kernel
* Gerar `compile_commands.json` (se o Bear estiver instalado)
* Preparar o projeto para funcionar corretamente no editor

O arquivo `.ko` será gerado dentro do diretório `kernel/`.

---

## 📦 Como carregar o módulo

Entre no diretório `kernel`:

```bash
cd kernel
```

Carregue o módulo:

```bash
sudo insmod testLKM.ko
```

Verifique se foi carregado:

```bash
lsmod | grep testLKM
```

Ou veja as mensagens do kernel:

```bash
dmesg | tail
```

---

## ❌ Como remover o módulo

```bash
sudo rmmod testLKM
```

E confira novamente:

```bash
lsmod | grep testLKM
```

---

## 🧹 Limpar arquivos de build

Na raiz do projeto:

```bash
make clean
```

---

## 🧠 Dicas úteis

* Sempre use `dmesg` para depurar mensagens do kernel
* Se o módulo não carregar, verifique erros com:

  ```bash
  dmesg | tail -n 50
  ```
* O projeto já está configurado para funcionar com **clangd** em:

  * VS Code
  * Neovim
  * Outros editores compatíveis




