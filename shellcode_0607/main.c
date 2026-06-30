#include "utils.h"

// 7.61 offsets
#define HV_REENTER_HYPERCORE 0x0000000062806380
#define HV_STACK_TABLE 0x000000006282E120
#define HV_PML4 0x000000006282E1A0
#define HV_ENTRY 0x000000006282E1B8
#define HV_MAIN 0x0000000000000E20
#define G_VM_TAB 0x0000000000027C80

#define MSR_APICBASE 0x01b
#define MSR_GSBASE 0xc0000101

#define DEFAULT_APIC_BASE 0xfee00000

#define APICBASE_ENABLED 0x00000800
#define APICBASE_BSP 0x00000100

#define NESTED_CTRL_NP_ENABLE 0x1

__attribute__((section(".entry_point"), naked)) uint32_t main(void) {
  uint64_t hv_pml4 = *(uint64_t *)HV_PML4;
  uint64_t hv_base = *(uint64_t *)HV_ENTRY - HV_MAIN;

  uintptr_t *g_vm_tab =
      (uintptr_t *)vtophys_custom(hv_base + G_VM_TAB, hv_pml4);
  for (int i = 0; i < 16; i++) {
    uintptr_t vc = vtophys_custom(g_vm_tab[i], hv_pml4);
    uintptr_t vmcb = vtophys_custom(*(uintptr_t *)(vc + 0x08), hv_pml4);

    if (i == 0) {
      // Restore guest_apic_base.
      *(uint64_t *)(vc + 0xe8) =
          DEFAULT_APIC_BASE | APICBASE_ENABLED | APICBASE_BSP;
    }

    // Disable nested paging.
    *(uint64_t *)(vmcb + 0x90) &= ~NESTED_CTRL_NP_ENABLE;
  }

  // Restore host apic base.
  wrmsr(MSR_APICBASE, DEFAULT_APIC_BASE | APICBASE_ENABLED | APICBASE_BSP);

  // Restore gs base.
  wrmsr(MSR_GSBASE, ((uint64_t *)HV_STACK_TABLE)[0] + 0x1000);

  // Reenter hypercore.
  void (*hv_reenter_hypercore)(void) = (void *)HV_REENTER_HYPERCORE;
  hv_reenter_hypercore();
  while (1)
    ;
}
