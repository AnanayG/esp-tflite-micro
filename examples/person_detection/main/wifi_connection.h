// Expose a C friendly interface for main functions.
#ifdef __cplusplus
extern "C" {
#endif

void initialize_nvs();

void wifi_connect();

#ifdef __cplusplus
}
#endif