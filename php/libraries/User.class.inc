<?php
/**
 * This file contains the Loris user class
 *
 * PHP Version 5
 *
 * @category Main
 * @package  Main
 * @author   Alex Zijdenbos <zijdenbos@example.com>
 * @license  http://www.gnu.org/licenses/gpl-3.0.txt GPLv3
 * @link     https://www.github.com/aces/Loris-Trunk/
 */

require_once 'NDB_Factory.class.inc';
require_once 'UserPermissions.class.inc';
require_once 'CouchDB.class.inc';

/**
 * User class. This deals with any User management except permissions,
 * which is contained in the UserPermissions base class.
 *
 * @category Main
 * @package  Main
 * @author   Alex Zijdenbos <zijdenbos@example.com>
 * @license  http://www.gnu.org/licenses/gpl-3.0.txt GPLv3
 * @link     https://www.github.com/aces/Loris-Trunk/
 */
class User extends UserPermissions
{
    /**
     * Stores user information
     * @var    array
     * @access private
     */
    var $userInfo = array();

    /**
     * Constructor
     *
     * Selects a user and its permissions
     *
     * @param string $username Identifies the user
     *
     * @return User
     * @access public
     */
    static function &factory($username)
    {
        $obj = new User;

        // right off, set the username
        $obj->userInfo['UserID'] = $username;

        // get the user's permissions
        $success = $obj->select($username);

        // create DB object
        $DB =& Database::singleton();

        // get user data from database
        $query = "SELECT users.*, psc.Name AS Site FROM users
            LEFT JOIN psc ON (users.centerID=psc.CenterID)
            WHERE users.UserID = :UID";

        $row = $DB->pselectRow($query, array('UID' => $username));

        // store user data in object property
        $obj->userInfo = $row;

        return $obj;
    }


    /**
     * Singleton method. Retrieve the user passed as parameter, but only
     * 1 instance.
     *
     * @param string $username Identifies the user
     *
     * @return User
     * @access public
     * @static
     */
    static function &singleton($username = null)
    {
        static $instance;
        if (is_null($instance)) {
            $instance = User::factory($username);
        }
        return $instance;
    }


    /**
     * Inserts a user
     *
     * @param array $set The array formatted for use in a Database call
     *
     * @return void
     * @access public
     * @static
     */
    function insert($set)
    {
        // create DB object
        $DB =& Database::singleton();

        $success = $DB->insert('users', $set);

        return true;
    }


    /**
     * Updates a user
     *
     * @param array $set The array formatted for use in a Database call
     *
     * @return void
     * @access public
     */
    function update($set)
    {
        // create DB object
        $DB =& Database::singleton();

        $success = $DB->update(
            'users',
            $set,
            array('UserID' => $this->userInfo['UserID'])
        );

        return true;
    }

    /**
     * Gets user data
     *
     * @param string $var Name of variable to get
     *
     * @note   Call without any parameters to get the entire user data array
     * @return mixed
     * @access public
     */
    function getData($var = null)
    {
        if (is_null($var)) {
            return $this->userInfo;
        } else {
            return $this->userInfo[$var];
        }
    }

    /**
     * Get users real name
     *
     * @return string
     */
    function getFullname()
    {
        return $this->userInfo['Real_name'];
    }

    /**
     * Get the user's username
     *
     * @return string
     */
    function getUsername()
    {
        return $this->userInfo['UserID'];
    }

    /**
     * Get the user's site's name
     *
     * @return string
     */
    function getSiteName()
    {
        return $this->userInfo['Site'];
    }

    /**
     * Get the user's site's ID number
     *
     * @return integer
     */
    function getCenterID()
    {
        return $this->userInfo['CenterID'];
    }

    /**
     * Unknown what this does. Maybe it should be removed?
     * - Dave
     *
     * @return boolean
     */
    function isMultiSiteEnabled()
    {
        // if $userID is not passed,then look-up current user
        // if CenterID = 0 then admin staff
        // like dcc ccc nih or similar
        // change 2002-10-23 Dario
        $config =& NDB_Config::singleton();
        return in_array(
            $this->userInfo['CenterID'],
            $config->getSetting('multiSiteEnabledCenters')
        );
    }

    /**
     * Checks that the user's email is valid
     *
     * @return boolean
     * @access public
     */
    function isEmailValid()
    {
        if (!filter_var($this->userInfo['Email'], FILTER_VALIDATE_EMAIL)) {
            return false;
        };
        return true;
    }


    /**
     * Makes an md5 hash with a salt
     *
     * @param string $word The word to hash
     *
     * @return string
     * @access public
     * @static
     */
    static function MD5Salt($word)
    {
        // get a two-character hexadecimal salt from 10 to ff
        $salt = dechex(rand(16, 255));

        // return the salted md5 hash
        return $salt . md5($salt . $word);
    }


    /**
     * Checks an md5 salt
     *
     * @param string $word The word to compare to the hash
     * @param string $hash The hash
     *
     * @return boolean
     * @access public
     * @static
     */
    static function MD5Unsalt($word, $hash)
    {
        // get the first two characters of the hash
        $salt = substr($hash, 0, 2);

        // return whether the hash matches the word
        return ($salt . md5($salt . $word) == $hash);
    }


    /**
     * Generates a random alphanumeric password
     *
     * @param int $length Length of password to generate
     *
     * @return string
     * @access public
     * @static
     */
    function newPassword($length = 8)
    {
        // start with a blank password
        $password = '';

        // define possible characters
        $possible = '0123456789bcdfghjkmnpqrstvwxyz';

        // length of possible characters minus one
        $possible_cnt = 29; //strlen($possible) - 1;

        // add random characters to $password until $length is reached
        for ($i = 0; $i < $length; $i++) {
            $password .= substr($possible, mt_rand(0, $possible_cnt), 1);
        }

        return $password;
    }


    /**
     * Check password strength
     *
     * If the password fails a comparison, it is not strong
     *
     * @param string $password  Password to check
     * @param array  $values    Values that can not be in the password
     *                          should contain ie. the user's name
     * @param array  $operators Operators used in comparison of values
     *                          array
     *
     * @return boolean
     * @access public
     * @static
     */
    function isPasswordStrong($password, $values = array(), $operators = array())
    {
        $CharTypes = 0;
        // less than 6 characters
        if (strlen($password) < 8) {
            return false;
        }

        // nothing but letters
        if (!preg_match('/[^A-Za-z]/', $password)) {
            return false;
        }

        // nothing but numbers
        if (!preg_match('/[^0-9]/', $password)) {
            return false;
        }

        // preg_match returns 1 on match, 0 on non-match
        $CharTypes += preg_match('/[0-9]+/', $password);
        $CharTypes += preg_match('/[A-Za-z]+/', $password);
        $CharTypes += preg_match('/[!\\\$\^@#%&\*\(\)]+/', $password);
        if ($CharTypes < 3) {
            return false;
        }

        // compare password to values
        foreach ($values as $key => $value) {
            $function = create_function(
                '$a, $b',
                'return $a ' . $operators[$key] . ' $b;'
            );
            // if the comparison fails
            if (!$function($password, $value)) {
                return false;
            }
        }

        return true;
    }
    /**
     * Check if user belongs to DCC
     *
     * @return boolean
     */
    function isUserDCC()
    {
        //DCC site = 1 by LORIS convention
        return ($this->userInfo['CenterID']== 1);
    }

    /**
     * Updates the password for this user into the appropriate field in the
     * database.
     *
     * @param string $password The plain text password to be hashed and saved.
     *
     * @return none
     */
    function updatePassword($password)
    {
        // Check if the PHP 5.5 password hashing API is available.
        $php55 = false;
        if (function_exists("password_verify")) {
            $php55 = true;
        }

        $expiry = date(
            'Y-m-d',
            mktime(0, 0, 0, date('n') + 6, date('j'), date('Y'))
        );

        $updateArray = array('Password_expiry' => $expiry);

        // Update the appropriate password field, null the other one
        // for security.
        if ($php55) {
            $updateArray['Password_hash'] = password_hash(
                $password,
                PASSWORD_DEFAULT
            );
            $updateArray['Password_MD5']  = null;
        } else {
            $updateArray['Password_hash'] = null;
            $updateArray['Password_MD5']  = User::MD5Salt($password);
        }

        $this->update($updateArray);

        $config       =& NDB_Config::singleton();
        $couch_config = $config->getSetting("CouchDB");
        if ($couch_config['SyncAccounts'] === "true") {
            $this->updateCouchUser($password);
        }
    }
    /**
     * Updates the CouchDB user for this user to synchronize
     * the password with Loris.
     *
     * @param string $password Plaintext password to synchronize
     *
     * @return none
     */
    function updateCouchUser($password)
    {
        $factory = NDB_Factory::singleton();
        $cdb     = $factory->CouchDB();
        $config  = $factory->Config();

        $c_config = $config->getSetting('CouchDB');
        if ($c_config['SyncAccounts'] !== 'true') {
            return false;
        }
        $username = $this->getUsername();
        $authkey  = base64_encode(
            $c_config['admin']
            . ':'
            . $c_config['adminpass']
        );

        $auth     = array('Authorization' => 'Basic ' . $authkey);
        $userjson = $cdb->_getURL(
            "/_users/org.couchdb.user:" . $username,
            'GET',
            $auth
        );
        $userobj  = json_decode($userjson, true);
        if (isset($userobj['error']) && $userobj['error'] == 'not_found') {
            $userobj = array(
                        '_id'   => 'org.couchdb.user:' . $username,
                        'type'  => 'user',
                        'name'  => $username,
                        'roles' => array(
                                    'dqg',
                                    $c_config['database'],
                                   ),
                       );
        }
        $userobj['password'] = $password;

        $response = $cdb->_postURL(
            "PUT /_users/org.couchdb.user:" . $username,
            json_encode($userobj)
        );
    }
}
?>
