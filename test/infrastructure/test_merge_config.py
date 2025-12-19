#!/usr/bin/env python3
"""
Tests for merge_config.py (located in /script)
"""

import unittest
import tempfile
import shutil
import os
import sys
import yaml

# Import the merge script 
sys.path.insert(0, os.path.dirname(__file__))

class TestConfigMerge(unittest.TestCase):
    
    def setUp(self):
        """Create temporary directories for each test"""
        self.test_dir = tempfile.mkdtemp()
        self.defaults_dir = os.path.join(self.test_dir, 'defaults')
        self.config_dir = os.path.join(self.test_dir, 'config')
        os.makedirs(self.defaults_dir)
        os.makedirs(self.config_dir)
    
    def tearDown(self):
        """Clean up temporary directories"""
        shutil.rmtree(self.test_dir)
    
    def write_yaml(self, path, data):
        """Helper to write YAML file"""
        with open(path, 'w') as f:
            yaml.dump(data, f)
    
    def read_yaml(self, path):
        """Helper to read YAML file"""
        with open(path, 'r') as f:
            return yaml.safe_load(f)
    
    def run_merge(self):
        """Run the merge script"""
        import subprocess
        
        # Find merge_config.py relative to test file
        test_dir = os.path.dirname(__file__)                    # test/infrastructure/
        test_parent = os.path.dirname(test_dir)                 # test/
        repo_root = os.path.dirname(test_parent)                # (root)
        script_path = os.path.join(repo_root, 'script', 'merge_config.py')
        
        user_yml = os.path.join(self.config_dir, 'user.yml')
        output_yml = os.path.join(self.config_dir, 'config.yml')
        debug_yml = os.path.join(self.config_dir, 'config.debug.yml')
        
        result = subprocess.run([
            'python3', script_path,
            self.defaults_dir, user_yml, output_yml, debug_yml
        ])
        self.assertEqual(result.returncode, 0, "Merge script failed")
        
        return output_yml, debug_yml
    
    def test_defaults_only(self):
        """Test merge with only default.yml"""
        default_config = {
            'process': {'detection': {'pfa': 0.001}},
            'network': {'ip': '0.0.0.0'}
        }
        self.write_yaml(os.path.join(self.defaults_dir, 'default.yml'), default_config)
        self.write_yaml(os.path.join(self.defaults_dir, 'forced.yml'), {})
        
        output_yml, debug_yml = self.run_merge()
        
        # user.yml should be created
        user_yml = os.path.join(self.config_dir, 'user.yml')
        self.assertTrue(os.path.exists(user_yml))
        
        # Output should match defaults
        output = self.read_yaml(output_yml)
        self.assertEqual(output['process']['detection']['pfa'], 0.001)
        self.assertEqual(output['network']['ip'], '0.0.0.0')
    
    def test_user_override(self):
        """Test that user.yml overrides defaults"""
        default_config = {
            'process': {'detection': {'pfa': 0.001, 'minDelay': 5}},
            'network': {'ip': '0.0.0.0'}
        }
        user_config = {
            'process': {'detection': {'pfa': 0.0001}}  # Override pfa only
        }
        
        self.write_yaml(os.path.join(self.defaults_dir, 'default.yml'), default_config)
        self.write_yaml(os.path.join(self.defaults_dir, 'forced.yml'), {})
        self.write_yaml(os.path.join(self.config_dir, 'user.yml'), user_config)
        
        output_yml, _ = self.run_merge()
        output = self.read_yaml(output_yml)
        
        # User override should apply
        self.assertEqual(output['process']['detection']['pfa'], 0.0001)
        # Other defaults should remain
        self.assertEqual(output['process']['detection']['minDelay'], 5)
        self.assertEqual(output['network']['ip'], '0.0.0.0')
    
    def test_forced_override(self):
        """Test that forced.yml overrides everything"""
        default_config = {
            'process': {'detection': {'pfa': 0.001}},
            'network': {'ip': '0.0.0.0'}
        }
        user_config = {
            'process': {'detection': {'pfa': 0.0001}}
        }
        forced_config = {
            'process': {'detection': {'pfa': 0.00001}}  # Force different value
        }
        
        self.write_yaml(os.path.join(self.defaults_dir, 'default.yml'), default_config)
        self.write_yaml(os.path.join(self.defaults_dir, 'forced.yml'), forced_config)
        self.write_yaml(os.path.join(self.config_dir, 'user.yml'), user_config)
        
        output_yml, _ = self.run_merge()
        output = self.read_yaml(output_yml)
        
        # Forced should override user
        self.assertEqual(output['process']['detection']['pfa'], 0.00001)
    
    def test_deep_merge(self):
        """Test deep merging of nested dictionaries"""
        default_config = {
            'location': {
                'rx': {'latitude': 0, 'longitude': 0, 'altitude': 0},
                'tx': {'latitude': 0, 'longitude': 0, 'altitude': 0}
            }
        }
        user_config = {
            'location': {
                'rx': {'latitude': 37.7749}  # Only override latitude
            }
        }
        
        self.write_yaml(os.path.join(self.defaults_dir, 'default.yml'), default_config)
        self.write_yaml(os.path.join(self.defaults_dir, 'forced.yml'), {})
        self.write_yaml(os.path.join(self.config_dir, 'user.yml'), user_config)
        
        output_yml, _ = self.run_merge()
        output = self.read_yaml(output_yml)
        
        # Deep merge should preserve other values
        self.assertEqual(output['location']['rx']['latitude'], 37.7749)
        self.assertEqual(output['location']['rx']['longitude'], 0)
        self.assertEqual(output['location']['rx']['altitude'], 0)
        self.assertIn('tx', output['location'])
    
    def test_debug_copy_created(self):
        """Test that debug copy is written"""
        default_config = {'test': 'value'}
        self.write_yaml(os.path.join(self.defaults_dir, 'default.yml'), default_config)
        self.write_yaml(os.path.join(self.defaults_dir, 'forced.yml'), {})
        
        output_yml, debug_yml = self.run_merge()
        
        # Debug copy should exist
        self.assertTrue(os.path.exists(debug_yml))
        
        # Debug copy should match output
        output = self.read_yaml(output_yml)
        debug = self.read_yaml(debug_yml)
        self.assertEqual(output, debug)
    
    def test_empty_user_config(self):
        """Test with empty user.yml"""
        default_config = {'process': {'detection': {'pfa': 0.001}}}
        
        self.write_yaml(os.path.join(self.defaults_dir, 'default.yml'), default_config)
        self.write_yaml(os.path.join(self.defaults_dir, 'forced.yml'), {})
        self.write_yaml(os.path.join(self.config_dir, 'user.yml'), {})
        
        output_yml, _ = self.run_merge()
        output = self.read_yaml(output_yml)
        
        # Should just use defaults
        self.assertEqual(output['process']['detection']['pfa'], 0.001)

    def test_missing_forced_config(self):
        """Test that missing forced.yml doesn't break merge"""
        default_config = {'process': {'detection': {'pfa': 0.001}}}
        
        self.write_yaml(os.path.join(self.defaults_dir, 'default.yml'), default_config)
        # Don't create forced.yml
        
        output_yml, _ = self.run_merge()
        output = self.read_yaml(output_yml)
        
        # Should still work with just defaults
        self.assertEqual(output['process']['detection']['pfa'], 0.001)

    def test_invalid_user_yaml(self):
        """Test handling of malformed YAML in user.yml"""
        default_config = {'process': {'detection': {'pfa': 0.001}}}
        
        self.write_yaml(os.path.join(self.defaults_dir, 'default.yml'), default_config)
        self.write_yaml(os.path.join(self.defaults_dir, 'forced.yml'), {})
        
        # Write invalid YAML
        with open(os.path.join(self.config_dir, 'user.yml'), 'w') as f:
            f.write("invalid: yaml: content:\n  - broken")
        
        # Should fail gracefully
        with self.assertRaises(Exception):
            self.run_merge()

    def test_list_replacement_not_merge(self):
        """Test that lists are replaced, not merged"""
        default_config = {
            'list_items': [1, 2, 3],
            'other': 'value'
        }
        user_config = {
            'list_items': [4, 5]
        }
        
        self.write_yaml(os.path.join(self.defaults_dir, 'default.yml'), default_config)
        self.write_yaml(os.path.join(self.defaults_dir, 'forced.yml'), {})
        self.write_yaml(os.path.join(self.config_dir, 'user.yml'), user_config)
        
        output_yml, _ = self.run_merge()
        output = self.read_yaml(output_yml)
        
        # List should be replaced, not merged
        self.assertEqual(output['list_items'], [4, 5])

    def test_actual_config_files(self):
        """Test merge with actual config files from the repo"""
        import subprocess
        
        # Use actual config files from repo
        repo_root = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
        actual_defaults_dir = os.path.join(repo_root, 'config')
        
        # Verify config files exist
        self.assertTrue(os.path.exists(os.path.join(actual_defaults_dir, 'default.yml')))
        
        # Create test user.yml
        user_yml = os.path.join(self.config_dir, 'user.yml')
        user_config = {'network': {'node_id': 'test-node'}}
        self.write_yaml(user_yml, user_config)
        
        output_yml = os.path.join(self.config_dir, 'config.yml')
        debug_yml = os.path.join(self.config_dir, 'config.debug.yml')
        
        # Run merge with actual config files
        script_path = os.path.join(repo_root, 'script', 'merge_config.py')
        result = subprocess.run([
            'python3', script_path,
            actual_defaults_dir, user_yml, output_yml, debug_yml
        ])
        
        self.assertEqual(result.returncode, 0, "Merge with actual configs failed")
        
        # Verify output is valid YAML and has expected structure
        output = self.read_yaml(output_yml)
        self.assertIn('process', output)
        self.assertIn('network', output)
        self.assertEqual(output['network']['node_id'], 'test-node')

if __name__ == '__main__':
    unittest.main()