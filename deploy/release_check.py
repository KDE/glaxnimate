#!/usr/bin/env python3
import re
import sys
import time
import hashlib
import argparse
import requests
from urllib.parse import urljoin
from xml.etree import ElementTree


class GitlabApi:
    def __init__(self, project_id="19921167"):
        self.api_url = "https://gitlab.com/api/v4/projects/%s/repository" % project_id

    def get(self, *url, **kwargs):
        #can_fail = not kwargs.pop("can_fail", False)
        url = "/".join((self.api_url,) + url)
        res = requests.request("get", url, **kwargs)
        #if can_fail:
        #res.raise_for_status()
        return (res.json(), res.status_code)


def download(indent, url):
    res = requests.get(url)
    if res.status_code != 200:
        error(indent, "Failed to fetch %s (%s)" % (url, res.status_code))
        return None
    time.sleep(0.1)
    return res.content


def log(indent, msg):
    print(" " * (indent*4) + msg)


def error(indent, msg):
    global retcode
    retcode += 1
    sys.stderr.write(" " * (indent*4) + "\x1b[31m%s\x1b[m\n" % msg)


def get_link(td, row, col):
    found = None

    for a in td:
        if a.tag == "a":
            if found is not None:
                error(2, "Too many links in row %s col %s" % (row, col))
                return None
            found = a

    if found is None:
        error(2, "No link in row %s col %s" % (row, col))
        return None

    return [found.attrib["href"], found.text]


def check_download_table(description, rel_url=""):
    match_s = re.search("<table>", description)
    match_e = re.search("</table>", description)
    if not match_s or not match_e:
        error(1, "No download table")
        return

    html_text = description[match_s.start(0):match_e.end(0)]
    try:
        html = ElementTree.fromstring(html_text)
    except ElementTree.ParseError as e:
        print(html_text)
        error(1, "Invalid download table: %s" % e)
        return

    for i, e in enumerate(html):
        if e.tag != "tr":
            error(1, "Expected <tr> for row %s" % i)
            continue
        if len(e) != 3:
            error(1, "Unexpected number of elements in row %s" % i)
            continue
        if i == 0:
            if e[0].tag != "th":
                error(1, "First row should be headers")
            continue
        if any(c.tag != "td" for c in e):
            error(1, "Invalid elements in row %s" % i)

        links = [get_link(td, i, col) for col, td in enumerate(e)]
        if any(x is None for x in links):
            continue

        download_link, download_name = links[0]
        sha_link, sha_algo = links[1]
        notes_link = links[2][0]
        log(1, download_name)

        hasher = getattr(hashlib, sha_algo.lower(), None)
        if not hasher:
            error(2, "Unknown hashing algorithm: %s" % sha_algo)
            continue

        checksum = download(2, sha_link)
        if checksum is None:
            continue

        data = download(2, download_link)
        if data is None:
            continue

        if not notes_link.startswith("#"):
            download(2, urljoin(rel_url, notes_link))

        eval_hash = hasher(data).hexdigest()
        dl_hash = re.search(b'^[0-9a-f]+', checksum).group(0).decode("utf-8")
        if dl_hash != eval_hash:
            error(2, "Hash mismatch: got %s, should be %s" % (dl_hash, eval_hash))


def check_tag():
    log(0, "Checking tag")
    response, status = api.get("tags", ns.version)
    if status != 200:
        error(1, "No tag")
        return

    if "release" not in response or "description" not in response["release"]:
        error(1, "No release")
        return

    log(0, "Validating Release Page Downloads")
    description = response["release"]["description"]
    check_download_table(description)


def check_download_page():
    log(0, "Validating Website Download Page")
    url = "https://glaxnimate.mattbas.org/download"
    html = download(1, url)
    if not html:
        return
    check_download_table(html.decode("utf-8"), url)


retcode = 0
parser = argparse.ArgumentParser()
parser.add_argument("version")

ns = parser.parse_args()

api = GitlabApi()
check_tag()
check_download_page()
sys.exit(retcode)