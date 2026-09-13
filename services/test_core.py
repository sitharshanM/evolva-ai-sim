import copy
import unittest
import sqlite3
from client import enqueue
from core import validate_archive, lexical_vector, analyze, chronicle


class CoreTests(unittest.TestCase):
    def setUp(self):
        self.archive = {'format':'aeon-replay-1','seed':42,'metrics':{'year':2027},'runtime':{'events':[
            {'id':1,'cause':0,'year':2027,'type':'COMMAND','description':'RESEARCH'},
            {'id':2,'cause':1,'year':2027,'type':'APPLIED','description':'RESEARCH'}]}}

    def test_content_addressed_identity(self):
        self.assertEqual(validate_archive(self.archive), validate_archive(copy.deepcopy(self.archive)))

    def test_missing_parent_rejected(self):
        self.archive['runtime']['events'][1]['cause'] = 12
        with self.assertRaises(ValueError): validate_archive(self.archive)

    def test_duplicate_id_rejected(self):
        self.archive['runtime']['events'][1]['id'] = 1
        with self.assertRaises(ValueError): validate_archive(self.archive)

    def test_vector_is_normalized_and_repeatable(self):
        vector = lexical_vector('research investment')
        self.assertEqual(vector, lexical_vector('research investment'))
        self.assertAlmostEqual(sum(x*x for x in vector), 1)

    def test_projections(self):
        self.assertEqual(analyze(self.archive)['event_counts']['APPLIED'], 1)
        self.assertEqual(chronicle(self.archive)['eras'][0]['actions'], ['RESEARCH'])

    def test_local_outbox_is_idempotent(self):
        with sqlite3.connect(':memory:') as database:
            enqueue(database, self.archive)
            enqueue(database, self.archive)
            self.assertEqual(database.execute('SELECT count(*) FROM outbox').fetchone()[0], 1)


if __name__ == '__main__': unittest.main()
