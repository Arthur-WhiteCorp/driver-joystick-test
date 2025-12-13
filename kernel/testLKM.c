/*
 * O módulo do kernel mais simples.
 */
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");                                     // Tipo de licença -- afeta o comportamento do tempo de execução
MODULE_AUTHOR("Arthur Silva Matias");                                 // Autor -- visivel quando usado o modinfo
MODULE_DESCRIPTION("Um simples driver para o devtitans");  // Descrição do módulo -- visível no modinfo
MODULE_VERSION("0.1");                                     // Versão do módulo

int init_module(void) {
    printk(KERN_INFO "Bem-vindo à criação do módulo ...\n");
    return 0;
}

void cleanup_module(void) {
    printk(KERN_INFO "O novo módulo foi removido ...\n");
}
