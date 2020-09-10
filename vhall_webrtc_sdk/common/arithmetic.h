#ifdef __cplusplus
extern "C" {
#endif



#ifndef WEI_ARITHMETIC_H
#define WEI_ARITHMETIC_H

#ifndef WIN32
#define WIN32
#ifdef NTLM_EXPORTS
#define NTLM_API __declspec(dllexport)
#else
#define NTLM_API __declspec(dllimport)
#endif
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG
#endif
/************************************* 通用方法 **************************************/
char * strtoupper(IN OUT char * a);
char chrtoupper(IN char a);
void unicode(IN char * src, IN int src_len, OUT char * dst, OUT int * dst_len);

/************************************ 算法：MD4和MD5 *********************************/
void MD4String (IN char * string,IN int len,OUT unsigned char * digest);
void MD5String (IN char * string,IN int len,OUT unsigned char * digest);
void HMAC_MD5(IN unsigned char * text, IN int text_len, IN unsigned char * key, IN int key_len, 
			  OUT unsigned char * digest);

/************************************* 算法：BASE64 **********************************/
void encode_base64(OUT char *dst, IN const char *src, IN int sz );
void decode_base64(IN char *src, IN int src_len, OUT char *dst,OUT int * dst_len);

/******************************* 算法：LM-HASH/NT-HASH/DES ***************************/
void algorithm_des(IN unsigned char * plaintext, IN unsigned char * key,OUT unsigned char * dst);
void algorithm_des_56key(IN unsigned char * plaintext, IN unsigned char * key,OUT unsigned char * dst);
void lm_hash(IN char * src, OUT unsigned char * dst, OUT int * dst_len);
void nt_hash(IN char * src, IN int is_unicode, OUT unsigned char * dst, OUT int * dst_len);
void ntlmv1_response(IN char * passwd,IN unsigned char * chanllenge,
					 OUT unsigned char * ntlm_response,OUT int * ntlm_response_len,
					 OUT unsigned char * lm_response, OUT int * lm_response_len);
					 //OUT unsigned char * dst, OUT int * dst_len);
void ntlmv2_response(IN char * passwd,IN char * user_name,IN char * domain,
					 IN unsigned char * chanllenge,IN unsigned char * target_info,
					 IN int target_info_len,IN unsigned char * client_nonce,
					 OUT unsigned char * dst,OUT int * ntlm_response_len,
					 OUT unsigned char * lm_response,   OUT int * lm_response_len);
void ntlmv2_session_response(IN char * passwd, IN unsigned char * chanllenge,
                     IN unsigned char * client_nonce,
                     OUT unsigned char * ntlm_response, OUT int * ntlm_response_len,
                     OUT unsigned char * lm_response,OUT int * lm_response_len);
#endif

#ifdef __cplusplus
}
#endif