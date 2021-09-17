# paging

## 关于分段与分页
分段和分页都是负责地址的转换

- 分段：逻辑地址->线性地址。经过分段解析后，地址被映射到一个线性地址空间（cpu可寻址的虚拟地址空间）
- 分页：线性地址->物理地址。经过分页解析后，地址被映射到真实的物理设备的地址

Cite:
> Chapter 3 explains how segmentation converts logical addresses to linear addresses. Paging (or linear-address translation) is the process of translating linear addresses so that they can be used to access memory or I/O devices. Paging translates each linear address to a physical address and determines, for each translation, what accesses to the linear address are allowed (the address’s access rights) and the type of caching used for such accesses (the address’s memory type).


## Ref
- _Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide. Chapter 4 Paging_