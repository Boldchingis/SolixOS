#ifndef SOLIX_NVME_H
#define SOLIXNVME_H

#include "types.h"
#include "kernel.h"

/**
 * SolixOS NVMe Driver Framework
 * Modern NVMe SSD support with high-performance I/O
 * Version: 2.0
 */

// NVMe specification constants
#define NVME_VERSION_1_0  0x000100
#define NVME_VERSION_1_1  0x000101
#define NVME_VERSION_1_2  0x000102
#define NVME_VERSION_1_3  0x000103
#define NVME_VERSION_1_4  0x000104

#define NVME_MAX_NAMESPACES 256
#define NVME_MAX_QUEUES 65535
#define NVME_MAX_IO_SIZE 4096

// NVMe opcode definitions
#define NVME_CMD_FLUSH        0x00
#define NVME_CMD_WRITE        0x01
#define NVME_CMD_READ         0x02
#define NVME_CMD_WRITE_UNCOR  0x04
#define NVME_CMD_COMPARE      0x05
#define NVME_CMD_WRITE_ZEROES 0x08
#define NVME_CMD_DSM          0x09
#define NVME_CMD_VERIFY       0x0C
#define NVME_CMD_RESV_REGISTER 0x0D
#define NVME_CMD_RESV_REPORT   0x0E
#define NVME_CMD_RESV_ACQUIRE  0x11
#define NVME_CMD_RESV_RELEASE  0x15

// Admin commands
#define NVME_ADMIN_DELETE_SQ      0x00
#define NVME_ADMIN_CREATE_SQ      0x01
#define NVME_ADMIN_GET_LOG_PAGE   0x02
#define NVME_ADMIN_DELETE_CQ      0x04
#define NVME_ADMIN_CREATE_CQ      0x05
#define NVME_ADMIN_IDENTIFY       0x06
#define NVME_ADMIN_ABORT_CMD      0x08
#define NVME_ADMIN_SET_FEATURES   0x09
#define NVME_ADMIN_GET_FEATURES   0x0A
#define NVME_ADMIN_ASYNC_EVENT    0x0C
#define NVME_ADMIN_NS_MGMT        0x0D
#define NVME_ADMIN_FW_COMMIT      0x10
#define NVME_ADMIN_FW_DOWNLOAD    0x11
#define NVME_ADMIN_FORMAT_NVM     0x80
#define NVME_ADMIN_SECURITY_SEND   0x81
#define NVME_ADMIN_SECURITY_RECV   0x82

// NVMe register offsets
#define NVME_REG_CAP     0x0000  // Controller Capabilities
#define NVME_REG_VS      0x0008  // Version
#define NVME_REG_INTMS   0x000C  // Interrupt Mask Set
#define NVME_REG_INTMC   0x0010  // Interrupt Mask Clear
#define NVME_REG_CC      0x0014  // Controller Configuration
#define NVME_REG_CSTS    0x001C  // Controller Status
#define NVME_REG_NSSR    0x0020  // NVM Subsystem Reset
#define NVME_REG_AQA     0x0024  // Admin Queue Attributes
#define NVME_REG_ASQ     0x0028  // Admin Submission Queue Base Address
#define NVME_REG_ACQ     0x0030  // Admin Completion Queue Base Address
#define NVME_REG_CMBLOC  0x0038  // Controller Memory Buffer Location
#define NVME_REG_CMBSZ   0x003C  // Controller Memory Buffer Size

// NVMe status codes
#define NVME_SC_SUCCESS              0x0000
#define NVME_SC_INVALID_OPCODE       0x0001
#define NVME_SC_INVALID_FIELD        0x0002
#define NVME_SC_CMDID_CONFLICT       0x0003
#define NVME_SC_DATA_XFER_ERROR      0x0004
#define NVME_SC_POWER_LOSS           0x0005
#define NVME_SC_INTERNAL             0x0006
#define NVME_SC_ABORT_REQ            0x0007
#define NVME_SC_ABORT_SQ_DELETION    0x0008
#define NVME_SC_ABORT_MISSING_FQ     0x0009
#define NVME_SC_ABORT_NO_DESC        0x000A
#define NVME_SC_ABORT_INVALID_QID    0x000B

// NVMe command structure
typedef struct nvme_command {
    uint8_t  opcode;
    uint8_t  flags;
    uint16_t command_id;
    uint32_t nsid;
    uint32_t cdw2;
    uint32_t cdw3;
    uint64_t metadata;
    uint64_t prp1;
    uint64_t prp2;
    uint32_t cdw10;
    uint32_t cdw11;
    uint32_t cdw12;
    uint32_t cdw13;
    uint32_t cdw14;
    uint32_t cdw15;
} __packed nvme_command_t;

// NVMe completion structure
typedef struct nvme_completion {
    uint32_t result0;
    uint32_t result1;
    uint16_t sq_head;
    uint16_t sq_id;
    uint16_t command_id;
    uint16_t status;
} __packed nvme_completion_t;

// NVMe queue structures
typedef struct nvme_queue {
    uint32_t* cq_db;     // Completion queue doorbell
    uint32_t* sq_db;     // Submission queue doorbell
    uint32_t q_depth;
    uint32_t cq_head;
    uint32_t sq_tail;
    uint32_t cq_phase;
    uint32_t sq_phase;
    
    nvme_command_t* sq_cmds;
    nvme_completion_t* cq_cmds;
    void* cq_dma_addr;
    void* sq_dma_addr;
    
    int cq_vector;
    int sq_vector;
} nvme_queue_t;

// NVMe controller identification
typedef struct nvme_id_ctrl {
    uint16_t vid;
    uint16_t ssvid;
    char sn[20];
    char mn[40];
    char fr[8];
    uint8_t rab;
    uint8_t ieee[3];
    uint8_t mic;
    uint8_t mdts;
    uint16_t cntlid;
    uint32_t ver;
    uint32_t rtd3r;
    uint32_t rtd3e;
    uint32_t oaes;
    uint32_t ctratt;
    uint16_t rrls;
    uint8_t rsvd1[9];
    uint8_t cntrltype;
    uint8_t fguid[16];
    uint16_t crdt1;
    uint16_t crdt2;
    uint16_t crdt3;
    uint8_t rsvd2[106];
    uint8_t nvmsr;
    uint8_t vwci;
    uint8_t mec;
    uint8_t rsvd3[31];
    uint8_t oacs;
    uint8_t acl;
    uint8_t aerl;
    uint8_t frmw;
    uint8_t lpa;
    uint8_t elpe;
    uint8_t npss;
    uint8_t avscc;
    uint8_t apsta;
    uint8_t wctemp;
    uint8_t cctemp;
    uint16_t mtfa;
    uint32_t hmpre;
    uint32_t hmmin;
    uint8_t tnvmcap[16];
    uint8_t unvmcap[16];
    uint32_t rpmbs;
    uint16_t edstt;
    uint8_t dsto;
    uint8_t fwug;
    uint16_t kas;
    uint16_t hctma;
    uint16_t mntemp;
    uint16_t mxtmp;
    uint32_t sanicap;
    uint32_t hmminds;
    uint16_t hmmaxd;
    uint8_t rsvd4[16];
    uint8_t sqes;
    uint8_t cqes;
    uint16_t maxcmd;
    uint32_t nn;
    uint16_t oncs;
    uint16_t fuses;
    uint8_t fna;
    uint8_t vwc;
    uint16_t awun;
    uint16_t awupf;
    uint8_t nvscc;
    uint8_t rsvd5;
    uint16_t acwu;
    uint8_t rsvd6[4];
    uint32_t sgls;
    uint8_t rsvd7[228];
    uint8_t subnqn[256];
    uint8_t rsvd8[768];
    uint8_t rsvd9[256];
} __packed nvme_id_ctrl_t;

// NVMe namespace identification
typedef struct nvme_id_ns {
    uint64_t nsze;
    uint64_t ncap;
    uint64_t nuse;
    uint8_t nsfeat;
    uint8_t nlbaf;
    uint8_t flbas;
    uint8_t mc;
    uint8_t dpc;
    uint8_t dps;
    uint8_t nmic;
    uint8_t rescap;
    uint8_t fpi;
    uint8_t dlfeat;
    uint16_t nawun;
    uint16_t nawupf;
    uint16_t nacwu;
    uint16_t nabs;
    uint16_t nabspf;
    uint16_t noiob;
    uint8_t nvmcap[16];
    uint8_t rsvd1[40];
    uint8_t nguid[16];
    uint8_t eui64[8];
    struct {
        uint16_t ms;
        uint8_t ds;
        uint8_t rp;
    } lbaf[16];
    uint8_t rsvd2[192];
    uint8_t vs[3712];
} __packed nvme_id_ns_t;

// NVMe controller structure
typedef struct nvme_ctrl {
    uint32_t id;
    void* regs;
    void* mmio_base;
    
    nvme_queue_t admin_q;
    nvme_queue_t* io_queues;
    uint32_t num_io_queues;
    
    nvme_id_ctrl_t id_ctrl;
    uint32_t max_q_depth;
    uint32_t max_segments;
    uint32_t max_transfer_size;
    
    uint32_t irq;
    uint32_t queue_count;
    uint32_t namespace_count;
    
    struct nvme_namespace* namespaces;
    struct nvme_ctrl* next;
    
    bool ready;
    uint32_t timeout;
} nvme_ctrl_t;

// NVMe namespace structure
typedef struct nvme_namespace {
    uint32_t ns_id;
    nvme_ctrl_t* ctrl;
    
    uint64_t size;
    uint64_t capacity;
    uint32_t block_size;
    uint32_t lba_shift;
    
    nvme_id_ns_t id_ns;
    
    char name[32];
    bool active;
    struct nvme_namespace* next;
} nvme_ns_t;

// NVMe request structure
typedef struct nvme_request {
    nvme_command_t cmd;
    void* buffer;
    uint32_t buffer_length;
    uint32_t actual_length;
    uint32_t status;
    
    void (*complete)(struct nvme_request* req);
    void* context;
    
    struct nvme_request* next;
} nvme_req_t;

// NVMe driver functions
API int nvme_init(void);
API int nvme_add_controller(void* mmio_base, uint32_t irq);
API int nvme_remove_controller(nvme_ctrl_t* ctrl);

API nvme_ns_t* nvme_get_namespace(nvme_ctrl_t* ctrl, uint32_t ns_id);
API int nvme_read_blocks(nvme_ns_t* ns, uint64_t lba, void* buffer, uint32_t blocks);
API int nvme_write_blocks(nvme_ns_t* ns, uint64_t lba, void* buffer, uint32_t blocks);
API int nvme_flush(nvme_ns_t* ns);

API nvme_req_t* nvme_alloc_request(void);
API void nvme_free_request(nvme_req_t* req);
API int nvme_submit_request(nvme_ctrl_t* ctrl, nvme_req_t* req);

API int nvme_create_io_queues(nvme_ctrl_t* ctrl, uint32_t num_queues);
API void nvme_delete_io_queues(nvme_ctrl_t* ctrl);

// NVMe debug and statistics
API void nvme_dump_controller_info(nvme_ctrl_t* ctrl);
API void nvme_dump_namespace_info(nvme_ns_t* ns);
API void nvme_get_statistics(nvme_ctrl_t* ctrl, uint32_t* stats);

#endif
