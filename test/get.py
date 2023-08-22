import requests, random

host = "http://localhost:6969"
test_idx = 0


def test(case, expected=200, headrs=None):
    print(case)
    if headrs is None:
        headrs = {"connection": "close"}
    else:
        headrs["connection"] = "close"
    response = requests.get(host, headers=headrs)
    if response.status_code != expected:
        print("{}: FAIL expected={}, received={}",
              ++test_idx, expected, response.status_code)
    else:
        print("SUCCESS")
    response.close()


def rand_str(randlen):
    base_sf = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-="
    rand = ''.join(random.sample(base_sf, 64))
    strlen = 64
    rand *= int(randlen / strlen)
    strlen *= int(randlen / strlen)
    rand += base_sf[0:randlen - strlen]
    return rand


test("Basic test")

headers = {}
max_req_len = 32768
max_header_len = 8192

headers["test"] = rand_str(max_header_len)
test("Headers too long", 431, headers)

headers.clear()
size = 0
while size < max_req_len:
    headers[str(size)] = rand_str(4096)
    size += 4096 + 6
test("Request too big", 413, headers)
