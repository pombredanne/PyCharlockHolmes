#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Test pycharlockholmes
#

import os
import unittest
from pycharlockholmes import (
    detect, get_supported_encodings, detect_all,
    set_strip_tags, get_strip_tags
)

TEST_DIR = os.path.dirname(os.path.realpath(__file__))


class EncodingDetectorTest(unittest.TestCase):
    TEST_FILES = {
        "py": [
            "file/test.py",
            {'confidence': 43, 'type': 'text', 'language': 'en',
             'encoding': 'ISO-8859-1'}
        ],
        "py1": [
            "file/test1.py",
            {'confidence': 80, 'type': 'text', 'encoding': 'UTF-8'}
        ],
        "lisp": [
            "file/test.lisp",
            {'confidence': 39, 'type': 'text', 'language': 'da',
             'encoding': 'ISO-8859-1'}
        ],
        "txt": [
            "file/test.txt",
            {'confidence': 16, 'type': 'text', 'language': 'en',
             'encoding': 'ISO-8859-1'}
        ],
        "c": [
            "file/test.c",
            {'confidence': 50, 'type': 'text', 'language': 'en',
             'encoding': 'ISO-8859-1'}
        ],
        "sh": [
            "file/test.sh",
            {'confidence': 21, 'type': 'text', 'language': 'es',
             'encoding': 'ISO-8859-1'}
        ],
        "elf": [
            "file/test",
            {'confidence': 100, 'type': 'binary'}
        ],
        "bz2": [
            "file/test.tar.bz2",
            {'confidence': 100, 'type': 'binary'}
        ],
        "gz": [
            "file/test.tar.gz",
            {'confidence': 100, 'type': 'binary'}
        ],
    }

    FIXTURE_FILES = [
        ['repl2.cljs',                  'ISO-8859-1',   'text'],
        ['cl-messagepack.lisp',         'ISO-8859-1',   'text'],
        ['sierpinski.ps',               'ISO-8859-1',   'text'],
        ['core.rkt',                    'UTF-8',        'text'],
        ['TwigExtensionsDate.es.yml',   'UTF-8',        'text'],
        ['laholator.py',                'UTF-8',        'text'],
        ['vimrc',                       'UTF-8',        'text'],
        ['AnsiGraph.psm1',              'UTF-16LE',     'text'],
        ['utf16be.html',                'UTF-16BE',     'text'],
        ['utf32le.html',                'UTF-32LE',     'text'],
        ['utf32be.html',                'UTF-32BE',     'text'],
        ['hello_world',                 None,           'binary'],
        ['octocat.png',                 None,           'binary'],
        ['octocat.jpg',                 None,           'binary'],
        ['octocat.psd',                 None,           'binary'],
        ['octocat.gif',                 None,           'binary'],
        ['octocat.ai',                  None,           'binary'],
        ['foo.pdf',                     None,           'binary'],
    ]

    def test_detection_works_as_expected(self):
        for test in self.TEST_FILES:
            file_path = self.TEST_FILES[test][0]
            file_result = self.TEST_FILES[test][1]
            content = open(file_path).read()
            test_result = detect(content)
            self.assertEqual(test_result, file_result)

    def test_detection_works_as_expected_2(self):
        for fixture in self.FIXTURE_FILES:
            filename, encoding, type_ = fixture

            file_path = 'fixtures/%s' % filename
            content = open(file_path).read()

            test_result = detect(content)

            self.assertEqual(type_, test_result['type'])

            check_encoding = (encoding == test_result.get('encoding', None))
            self.assertTrue(check_encoding)

    def test_detect_method(self):
        content = 'test'
        detected = detect(content)

        self.assertEqual('ISO-8859-1', detected['encoding'])

    def test_detect_accepts_encoding_hint(self):
        content = 'test'
        detected = detect(content, 'UTF-8')

        self.assertEqual('ISO-8859-1', detected['encoding'])

    def test_detect_all_method(self):
        content = 'test'
        detected_list = detect_all(content)

        self.assertTrue(isinstance(detected_list, list))
        encodings_list = [x['encoding'] for x in detected_list]
        self.assertEqual(['ISO-8859-1', 'ISO-8859-2', 'UTF-8'], encodings_list)

    def test_detect_all_method_accepts_encoding_hint(self):
        content = 'test'
        detected_list = detect_all(content, 'UTF-8')

        self.assertTrue(isinstance(detected_list, list))
        encodings_list = [x['encoding'] for x in detected_list]
        self.assertEqual(['ISO-8859-1', 'ISO-8859-2', 'UTF-8'], encodings_list)

    def test_strip_tags_flag(self):
        content = '<div ascii_attribute="some more ascii">λ, λ, λ</div>'
        set_strip_tags(True)
        detection = detect(content)
        self.assertEqual(get_strip_tags(), True)

        self.assertEqual('UTF-8', detection['encoding'])

        content2 = '<div ascii_attribute="some more ascii">λ, λ, λ</div>'
        set_strip_tags(False)
        detection2 = detect(content2)
        self.assertEqual(get_strip_tags(), False)

        self.assertEqual('UTF-8', detection2['encoding'])

    def test_has_list_of_supported_encodings(self):
        supported_encodings = get_supported_encodings()

        self.assertTrue(isinstance(supported_encodings, tuple))
        self.assertTrue('UTF-8' in supported_encodings)

    def test_binary(self):
        binary_files = [
            'octocat.ai',
            'octocat.png',
            'octocat.psd',
            'zip',
            'cube.stl',
            'dog.o',
            'foo.bin',
            'foo.pdf',
            'foo.png',
            'foo bar.jar',
            'git.deb',
            'git.exe',
            'github.po',
            'hello.pbc',
            'linguist.gem'
        ]
        binary_bin = '%s/binary' % TEST_DIR

        def get_detection(binary_file):
            path = '%s/%s' % (binary_bin, binary_file)

            detected = detect(open(path).read())
            return detected

        self.assertEqual(get_detection('git.deb')['type'], 'binary')
        self.assertEqual(get_detection('git.exe')['type'], 'binary')
        self.assertEqual(get_detection('hello.pbc')['type'], 'binary')
        self.assertEqual(get_detection('linguist.gem')['type'], 'binary')
        self.assertEqual(get_detection('octocat.ai')['type'], 'binary')
        self.assertEqual(get_detection('octocat.png')['type'], 'binary')
        self.assertEqual(get_detection('github.po')['type'], 'text')

if __name__ == '__main__':
    unittest.main()
