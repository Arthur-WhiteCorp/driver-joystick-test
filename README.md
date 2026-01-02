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

Para compilar o Driver para AOSP:
```bash
sudo apt-get install gcc-aarch64-linux-gnu
```

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
├── dto/                   (device tree overlay para o joystick do ESP32)
└── kernel/                      (código do módulo)
    ├── testLKM.c
    ├── Makefile
    └── compile_commands.json -> ../compile_commands.json
```

---

## ⚙️ Como compilar o módulo

Este projeto suporta duas plataformas de compilação:

### 🖥️ Para Linux Nativo (x86_64)

Para compilar o módulo na sua máquina de desenvolvimento (testes rápidos):

```bash
make
```

**Isso irá:**
* Compilar o módulo do kernel para sua distribuição atual
* Gerar `compile_commands.json` (se o Bear estiver instalado) para integração com IDEs
* Preparar o projeto para funcionar corretamente no editor

**Resultado:** O arquivo `.ko` será gerado dentro do diretório `kernel/`.

---

### 📱 Para AOSP (Android - ARM64)

Para compilar para dispositivos Android, primeiro configure o caminho do kernel no Makefile:

```makefile
# Atualize este caminho para onde seu kernel AOSP está localizado
AOSP_KERNEL = /caminho/para/seu/kernel-aosp
```

### 🚀 Fluxo de Trabalho Recomendado:

#### 1. **Configuração Inicial** (apenas primeira vez ou após mudanças no kernel)
```bash
make aosp-full
```
**O que faz:**
- Configura o kernel AOSP com `defconfig`
- Compila **todos os módulos do kernel** (necessário uma vez)
- Compila **nosso driver específico**

#### 2. **Desenvolvimento Iterativo** (uso diário - SUPER RÁPIDO)
```bash
make aosp
```
**O que faz:**
- Compila **apenas nosso driver** (presume kernel já compilado)
- Ideal para ciclo rápido de desenvolvimento/teste
- Mantém todas as configurações anteriores

### 🔧 Comandos Avançados:

```bash
# Apenas configurar o kernel (sem compilar)
make config

# Compilar apenas os módulos do kernel AOSP (sem nosso driver)
make aosp-kernel

# Limpar arquivos de compilação
make clean
```

---

## 🎯 Resumo do Fluxo para a Equipe:

| Quando usar | Comando | Tempo | Recomendado para |
|-------------|---------|-------|------------------|
| **Primeira configuração** | `make aosp-full` | ⏱️ Longo | Integração inicial |
| **Desenvolvimento diário** | `make aosp` | ⚡ Instantâneo | Iteração rápida |
| **Testes locais** | `make` | 🚀 Rápido | Debug no PC |
| **Problemas de build** | `make aosp-full` | ⏱️ Longo | Resolver dependências |

---

## 📝 Notas Importantes:

1. **Para desenvolvimento Android**, use sempre `make aosp` após a configuração inicial
2. O comando `make` padrão é apenas para testes rápidos no PC
3. Certifique-se de ter a toolchain ARM64 instalada:
   ```bash
   sudo apt-get install gcc-aarch64-linux-gnu
   ```
4. Mantenha o caminho do kernel AOSP atualizado no Makefile

---
🚀Carregue o módulo:

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

* Após o `make aosp-full` inicial, use sempre `make aosp` para builds rápidos durante o desenvolvimento! 
* Sempre use `dmesg` para depurar mensagens do kernel
* Se o módulo não carregar, verifique erros com:

  ```bash
  dmesg | tail -n 50
  ```
* O projeto já está configurado para funcionar com **clangd** em:

  * VS Code
  * Neovim
  * Outros editores compatíveis




